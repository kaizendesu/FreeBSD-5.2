/*
 * Copyright (c) 2001-2003
 *	Fraunhofer Institute for Open Communication Systems (FhG Fokus).
 *	All rights reserved.
 *
 * Author: Harti Brandt <harti@freebsd.org>
 *
 * Redistribution of this software and documentation and use in source and
 * binary forms, with or without modification, are permitted provided that
 * the following conditions are met:
 *
 * 1. Redistributions of source code or documentation must retain the above
 *    copyright notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE AND DOCUMENTATION IS PROVIDED BY FRAUNHOFER FOKUS
 * AND ITS CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * FRAUNHOFER FOKUS OR ITS CONTRIBUTORS  BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Begemot: bsnmp/snmpd/main.c,v 1.76 2003/01/28 13:44:35 hbb Exp $
 *
 * SNMPd main stuff.
 */
#include <sys/param.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "snmpmod.h"
#include "snmpd.h"
#include "tree.h"
#include "oid.h"

#define	PATH_PID	"/var/run/%s.pid"
#define PATH_CONFIG	"/etc/%s.config"

u_int32_t this_tick;	/* start of processing of current packet */
u_int32_t start_tick;	/* start of processing */

struct systemg systemg = {
	NULL,
	{ 8, { 1, 3, 6, 1, 4, 1, 1115, 7352 }},
	NULL, NULL, NULL,
	64 + 8 + 4,
	0
};
struct debug debug = {
	0,		/* dump_pdus */
	LOG_DEBUG,	/* log_pri */
	0,		/* evdebug */
};

struct snmpd snmpd = {
	2048,		/* txbuf */
	2048,		/* rxbuf */
	0,		/* comm_dis */
	0,		/* auth_traps */
	{0, 0, 0, 0},	/* trap1addr */
};
struct snmpd_stats snmpd_stats;

/* snmpSerialNo */
int32_t snmp_serial_no;

/* search path for config files */
const char *syspath = PATH_SYSCONFIG;

/* list of all loaded modules */
struct lmodules lmodules = TAILQ_HEAD_INITIALIZER(lmodules);

/* list of loaded modules during start-up in the order they were loaded */
static struct lmodules modules_start = TAILQ_HEAD_INITIALIZER(modules_start);

/* list of all known communities */
struct community_list community_list = TAILQ_HEAD_INITIALIZER(community_list);

/* list of all installed object resources */
struct objres_list objres_list = TAILQ_HEAD_INITIALIZER(objres_list);

/* community value generator */
static u_int next_community_index = 1;

/* list of all known ranges */
struct idrange_list idrange_list = TAILQ_HEAD_INITIALIZER(idrange_list);

/* identifier generator */
u_int next_idrange = 1;

/* list of all current timers */
struct timer_list timer_list = LIST_HEAD_INITIALIZER(timer_list);

/* list of file descriptors */
struct fdesc_list fdesc_list = LIST_HEAD_INITIALIZER(fdesc_list);

/* program arguments */
static char **progargs;
static int nprogargs;

/* current community */
u_int	community;
static struct community *comm;

/* list of all IP ports we are listening on */
struct snmp_port_list snmp_port_list =
    TAILQ_HEAD_INITIALIZER(snmp_port_list);

/* list of all local ports we are listening on */
struct local_port_list local_port_list =
    TAILQ_HEAD_INITIALIZER(local_port_list);

/* file names */
static char config_file[MAXPATHLEN + 1];
static char pid_file[MAXPATHLEN + 1];

/* event context */
static evContext evctx;

/* signal mask */
static sigset_t blocked_sigs;

/* signal handling */
static int work;
#define	WORK_DOINFO	0x0001
#define	WORK_RECONFIG	0x0002

/* oids */
static const struct asn_oid
	oid_snmpMIB = OIDX_snmpMIB,
	oid_begemotSnmpd = OIDX_begemotSnmpd,
	oid_coldStart = OIDX_coldStart,
	oid_authenticationFailure = OIDX_authenticationFailure;

const struct asn_oid oid_zeroDotZero = { 2, { 0, 0 }};

/* request id generator for traps */
u_int trap_reqid;

/* help text */
static const char usgtxt[] = "\
Begemot simple SNMP daemon. Copyright (c) 2001-2002 Fraunhofer Institute for\n\
Open Communication Systems (FhG Fokus). All rights reserved.\n\
usage: snmpd [-dh] [-c file] [-D options] [-I path] [-l prefix]\n\
             [-m variable=value] [-p file]\n\
options:\n\
  -d		don't daemonize\n\
  -h		print this info\n\
  -c file	specify configuration file\n\
  -D options	debugging options\n\
  -I path	system include path\n\
  -l prefix	default basename for pid and config file\n\
  -m var=val	define variable\n\
  -p file	specify pid file\n\
";

/* forward declarations */
static void snmp_printf_func(const char *fmt, ...);
static void snmp_error_func(const char *err, ...);
static void snmp_debug_func(const char *err, ...);
static void asn_error_func(const struct asn_buf *b, const char *err, ...);

/*
 * Allocate rx/tx buffer. We allocate one byte more for rx.
 */
void *
buf_alloc(int tx)
{
	void *buf;

	if ((buf = malloc(tx ? snmpd.txbuf : (snmpd.rxbuf + 1))) == NULL) {
		syslog(LOG_CRIT, "cannot allocate buffer");
		if (tx)
			snmpd_stats.noTxbuf++;
		else
			snmpd_stats.noRxbuf++;
		return (NULL);
	}
	return (buf);
}

/*
 * Return the buffer size. (one more for RX).
 */
size_t
buf_size(int tx)
{
	return (tx ? snmpd.txbuf : (snmpd.rxbuf + 1));
}

/*
 * Prepare a PDU for output
 */
void
snmp_output(struct snmp_v1_pdu *pdu, u_char *sndbuf, size_t *sndlen,
    const char *dest)
{
	struct asn_buf resp_b;

	resp_b.asn_ptr = sndbuf;
	resp_b.asn_len = snmpd.txbuf;

	if (snmp_pdu_encode(pdu, &resp_b) != 0) {
		syslog(LOG_ERR, "cannot encode message");
		abort();
	}
	if (debug.dump_pdus) {
		snmp_printf("%s <- ", dest);
		snmp_pdu_dump(pdu);
	}
	*sndlen = (size_t)(resp_b.asn_ptr - sndbuf);
}

/*
 * Send a PDU to a given port
 */
void
snmp_send_port(const struct asn_oid *port, struct snmp_v1_pdu *pdu,
    const struct sockaddr *addr, socklen_t addrlen)
{
	struct snmp_port *p;
	u_char *sndbuf;
	size_t sndlen;
	ssize_t len;

	TAILQ_FOREACH(p, &snmp_port_list, link)
		if (asn_compare_oid(port, &p->index) == 0)
			break;

	if (p == 0)
		return;

	if ((sndbuf = buf_alloc(1)) == NULL)
		return;

	snmp_output(pdu, sndbuf, &sndlen, "SNMP PROXY");

	if ((len = sendto(p->sock, sndbuf, sndlen, 0, addr, addrlen)) == -1)
		syslog(LOG_ERR, "sendto: %m");
	else if ((size_t)len != sndlen)
		syslog(LOG_ERR, "sendto: short write %zu/%zu",
		    sndlen, (size_t)len);

	free(sndbuf);
}

/*
 * SNMP input. Start: decode the PDU, find the community.
 */
enum snmpd_input_err
snmp_input_start(const u_char *buf, size_t len, const char *source,
    struct snmp_v1_pdu *pdu, int32_t *ip)
{
	struct asn_buf b;
	enum snmp_code code;
	enum snmpd_input_err ret;

	snmpd_stats.inPkts++;

	b.asn_cptr = buf;
	b.asn_len = len;
	code = snmp_pdu_decode(&b, pdu, ip);

	ret = SNMPD_INPUT_OK;
	switch (code) {

	  case SNMP_CODE_FAILED:
		snmpd_stats.inASNParseErrs++;
		return (SNMPD_INPUT_FAILED);

	  case SNMP_CODE_BADVERS:
		snmpd_stats.inBadVersions++;
		return (SNMPD_INPUT_FAILED);

	  case SNMP_CODE_BADLEN:
		if (pdu->type == SNMP_OP_SET)
			ret = SNMPD_INPUT_VALBADLEN;
		break;

	  case SNMP_CODE_OORANGE:
		if (pdu->type == SNMP_OP_SET)
			ret = SNMPD_INPUT_VALRANGE;
		break;

	  case SNMP_CODE_BADENC:
		if (pdu->type == SNMP_OP_SET)
			ret = SNMPD_INPUT_VALBADENC;
		break;

	  case SNMP_CODE_OK:
		break;
	}

	if (debug.dump_pdus) {
		snmp_printf("%s -> ", source);
		snmp_pdu_dump(pdu);
	}

	/*
	 * Look, whether we know the community
	 */
	TAILQ_FOREACH(comm, &community_list, link)
		if (comm->string != NULL &&
		    strcmp(comm->string, pdu->community) == 0)
			break;

	if (comm == NULL) {
		snmpd_stats.inBadCommunityNames++;
		snmp_pdu_free(pdu);
		if (snmpd.auth_traps)
			snmp_send_trap(&oid_authenticationFailure, NULL);
		return (SNMPD_INPUT_FAILED);
	}
	community = comm->value;

	/* update uptime */
	this_tick = get_ticks();

	return (ret);
}

/*
 * Will return only _OK or _FAILED
 */
enum snmpd_input_err
snmp_input_finish(struct snmp_pdu *pdu, const u_char *rcvbuf, size_t rcvlen,
    u_char *sndbuf, size_t *sndlen, const char *source,
    enum snmpd_input_err ierr, int32_t ivar, void *data)
{
	struct snmp_pdu resp;
	struct asn_buf resp_b, pdu_b;
	enum snmp_ret ret;

	resp_b.asn_ptr = sndbuf;
	resp_b.asn_len = snmpd.txbuf;

	pdu_b.asn_cptr = rcvbuf;
	pdu_b.asn_len = rcvlen;

	if (ierr != SNMPD_INPUT_OK) {
		/* error decoding the input of a SET */
		if (pdu->version == SNMP_V1)
			pdu->error_status = SNMP_ERR_BADVALUE;
		else if (ierr == SNMPD_INPUT_VALBADLEN)
			pdu->error_status = SNMP_ERR_WRONG_LENGTH;
		else if (ierr == SNMPD_INPUT_VALRANGE)
			pdu->error_status = SNMP_ERR_WRONG_VALUE;
		else
			pdu->error_status = SNMP_ERR_WRONG_ENCODING;

		pdu->error_index = ivar;

		if (snmp_make_errresp(pdu, &pdu_b, &resp_b) == SNMP_RET_IGN) {
			syslog(LOG_WARNING, "could not encode error response");
			snmpd_stats.silentDrops++;
			return (SNMPD_INPUT_FAILED);
		}

		if (debug.dump_pdus) {
			snmp_printf("%s <- ", source);
			snmp_pdu_dump(pdu);
		}
		*sndlen = (size_t)(resp_b.asn_ptr - sndbuf);
		return (SNMPD_INPUT_OK);
	}

	switch (pdu->type) {

	  case SNMP_PDU_GET:
		ret = snmp_get(pdu, &resp_b, &resp, data);
		break;

	  case SNMP_PDU_GETNEXT:
		ret = snmp_getnext(pdu, &resp_b, &resp, data);
		break;

	  case SNMP_PDU_SET:
		ret = snmp_set(pdu, &resp_b, &resp, data);
		break;

	  case SNMP_PDU_GETBULK:
		ret = snmp_getbulk(pdu, &resp_b, &resp, data);
		break;

	  default:
		ret = SNMP_RET_IGN;
		break;
	}

	switch (ret) {

	  case SNMP_RET_OK:
		/* normal return - send a response */
		if (debug.dump_pdus) {
			snmp_printf("%s <- ", source);
			snmp_pdu_dump(&resp);
		}
		*sndlen = (size_t)(resp_b.asn_ptr - sndbuf);
		snmp_pdu_free(&resp);
		return (SNMPD_INPUT_OK);

	  case SNMP_RET_IGN:
		/* error - send nothing */
		snmpd_stats.silentDrops++;
		return (SNMPD_INPUT_FAILED);

	  case SNMP_RET_ERR:
		/* error - send error response. The snmp routine has
		 * changed the error fields in the original message. */
		resp_b.asn_ptr = sndbuf;
		resp_b.asn_len = snmpd.txbuf;
		if (snmp_make_errresp(pdu, &pdu_b, &resp_b) == SNMP_RET_IGN) {
			syslog(LOG_WARNING, "could not encode error response");
			snmpd_stats.silentDrops++;
			return (SNMPD_INPUT_FAILED);
		} else {
			if (debug.dump_pdus) {
				snmp_printf("%s <- ", source);
				snmp_pdu_dump(pdu);
			}
			*sndlen = (size_t)(resp_b.asn_ptr - sndbuf);
			return (SNMPD_INPUT_OK);
		}
	}
	abort();
}



/*
 * File descriptor support
 */
static void
input(evContext ctx __unused, void *uap, int fd, int mask __unused)
{
	struct fdesc *f = uap;

	(*f->func)(fd, f->udata);
}

void
fd_suspend(void *p)
{
	struct fdesc *f = p;

	if (evTestID(f->id)) {
		(void)evDeselectFD(evctx, f->id);
		evInitID(&f->id);
	}
}

int
fd_resume(void *p)
{
	struct fdesc *f = p;
	int err;

	if (evTestID(f->id))
		return (0);
	if (evSelectFD(evctx, f->fd, EV_READ, input, f, &f->id)) {
		err = errno;
		syslog(LOG_ERR, "select fd %d: %m", f->fd);
		errno = err;
		return (-1);
	}
	return (0);
}

void *
fd_select(int fd, void (*func)(int, void *), void *udata, struct lmodule *mod)
{
	struct fdesc *f;
	int err;

	if ((f = malloc(sizeof(struct fdesc))) == NULL) {
		err = errno;
		syslog(LOG_ERR, "fd_select: %m");
		errno = err;
		return (NULL);
	}
	f->fd = fd;
	f->func = func;
	f->udata = udata;
	f->owner = mod;
	evInitID(&f->id);

	if (fd_resume(f)) {
		err = errno;
		free(f);
		errno = err;
		return (NULL);
	}

	LIST_INSERT_HEAD(&fdesc_list, f, link);

	return (f);
}

void
fd_deselect(void *p)
{
	struct fdesc *f = p;

	LIST_REMOVE(f, link);
	fd_suspend(f);
	free(f);
}

static void
fd_flush(struct lmodule *mod)
{
	struct fdesc *t, *t1;

	t = LIST_FIRST(&fdesc_list);
	while (t != NULL) {
		t1 = LIST_NEXT(t, link);
		if (t->owner == mod)
			fd_deselect(t);
		t = t1;
	}

}


/*
 * Input from UDP socket
 */
static void
do_input(int fd, const struct asn_oid *port_index,
    struct sockaddr *ret, socklen_t *retlen)
{
	u_char *resbuf, embuf[100];
	u_char *sndbuf;
	size_t sndlen;
	ssize_t len;
	struct snmp_v1_pdu pdu;
	enum snmpd_input_err ierr, ferr;
	enum snmpd_proxy_err perr;
	int32_t vi;

	if ((resbuf = buf_alloc(0)) == NULL) {
		(void)recvfrom(fd, embuf, sizeof(embuf), 0, ret, retlen);
		return;
	}
	if ((len = recvfrom(fd, resbuf, buf_size(0), 0, ret, retlen)) == -1) {
		free(resbuf);
		return;
	}
	if (len == 0) {
		free(resbuf);
		return;
	}
	if ((size_t)len == buf_size(0)) {
		free(resbuf);
		snmpd_stats.silentDrops++;
		snmpd_stats.inTooLong++;
		return;
	}

	/*
	 * Handle input
	 */
	ierr = snmp_input_start(resbuf, (size_t)len, "SNMP", &pdu, &vi);

	/* can't check for bad SET pdus here, because a proxy may have to
	 * check the access first. We don't want to return an error response
	 * to a proxy PDU with a wrong community */
	if (ierr == SNMPD_INPUT_FAILED) {
		free(resbuf);
		return;
	}

	/*
	 * If that is a module community and the module has a proxy function,
	 * the hand it over to the module.
	 */
	if (comm->owner != NULL && comm->owner->config->proxy != NULL) {
		perr = (*comm->owner->config->proxy)(&pdu, port_index,
		    ret, *retlen, ierr, vi);

		switch (perr) {

		  case SNMPD_PROXY_OK:
			free(resbuf);
			return;

		  case SNMPD_PROXY_REJ:
			break;

		  case SNMPD_PROXY_DROP:
			free(resbuf);
			snmp_pdu_free(&pdu);
			snmpd_stats.proxyDrops++;
			return;

		  case SNMPD_PROXY_BADCOMM:
			free(resbuf);
			snmp_pdu_free(&pdu);
			snmpd_stats.inBadCommunityNames++;
			if (snmpd.auth_traps)
				snmp_send_trap(&oid_authenticationFailure,
				    NULL);
			return;

		  case SNMPD_PROXY_BADCOMMUSE:
			free(resbuf);
			snmp_pdu_free(&pdu);
			snmpd_stats.inBadCommunityUses++;
			if (snmpd.auth_traps)
				snmp_send_trap(&oid_authenticationFailure,
				    NULL);
			return;
		}
	}

	/*
	 * Check type
	 */
	if (pdu.type == SNMP_PDU_RESPONSE ||
	    pdu.type == SNMP_PDU_TRAP ||
	    pdu.type == SNMP_PDU_TRAP2) {
		snmpd_stats.silentDrops++;
		snmpd_stats.inBadPduTypes++;
		snmp_pdu_free(&pdu);
		free(resbuf);
		return;
	}

	/*
	 * Check community
	 */
	if (community != COMM_WRITE &&
            (pdu.type == SNMP_PDU_SET || community != COMM_READ)) {
		snmpd_stats.inBadCommunityUses++;
		snmp_pdu_free(&pdu);
		free(resbuf);
		if (snmpd.auth_traps)
			snmp_send_trap(&oid_authenticationFailure, NULL);
		return;
	}

	/*
	 * Execute it.
	 */
	if ((sndbuf = buf_alloc(1)) == NULL) {
		snmpd_stats.silentDrops++;
		snmp_pdu_free(&pdu);
		free(resbuf);
		return;
	}
	ferr = snmp_input_finish(&pdu, resbuf, len, sndbuf, &sndlen, "SNMP",
	    ierr, vi, NULL);

	if (ferr == SNMPD_INPUT_OK) {
		if ((len = sendto(fd, sndbuf, sndlen, 0, ret, *retlen)) == -1)
			syslog(LOG_ERR, "sendto: %m");
		else if ((size_t)len != sndlen)
			syslog(LOG_ERR, "sendto: short write %zu/%zu",
			    sndlen, (size_t)len);
	}
	snmp_pdu_free(&pdu);
	free(sndbuf);
	free(resbuf);
}

static void
ssock_input(int fd, void *udata)
{
	struct snmp_port *p = udata;

	p->retlen = sizeof(p->ret);
	do_input(fd, &p->index, (struct sockaddr *)&p->ret, &p->retlen);
}

static void
lsock_input(int fd, void *udata)
{
	struct local_port *p = udata;

	p->retlen = sizeof(p->ret);
	do_input(fd, &p->index, (struct sockaddr *)&p->ret, &p->retlen);
}


/*
 * Create a UDP socket and bind it to the given port
 */
static int
init_snmp(struct snmp_port *p)
{
	struct sockaddr_in addr;
	u_int32_t ip;

	if ((p->sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "creating UDP socket: %m");
		return (SNMP_ERR_RES_UNAVAIL);
	}
	ip = (p->addr[0] << 24) | (p->addr[1] << 16) | (p->addr[2] << 8) |
	    p->addr[3];
	memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(ip);
	addr.sin_port = htons(p->port);
	addr.sin_family = AF_INET;
	addr.sin_len = sizeof(addr);
	if (bind(p->sock, (struct sockaddr *)&addr, sizeof(addr))) {
		if (errno == EADDRNOTAVAIL) {
			close(p->sock);
			p->sock = -1;
			return (SNMP_ERR_INCONS_NAME);
		}
		syslog(LOG_ERR, "bind: %s:%u %m", inet_ntoa(addr.sin_addr),
		    p->port);
		close(p->sock);
		p->sock = -1;
		return (SNMP_ERR_GENERR);
	}
	if ((p->id = fd_select(p->sock, ssock_input, p, NULL)) == NULL) {
		close(p->sock);
		p->sock = -1;
		return (SNMP_ERR_GENERR);
	}
	return (SNMP_ERR_NOERROR);
}


/*
 * Create a new SNMP Port object and start it, if we are not
 * in initialisation mode. The arguments are in host byte order.
 */
int
open_snmp_port(u_int8_t *addr, u_int32_t port, struct snmp_port **pp)
{
	struct snmp_port *snmp, *p;
	int err;

	if (port > 0xffff)
		return (SNMP_ERR_NO_CREATION);
	if ((snmp = malloc(sizeof(*snmp))) == NULL)
		return (SNMP_ERR_GENERR);
	snmp->addr[0] = addr[0];
	snmp->addr[1] = addr[1];
	snmp->addr[2] = addr[2];
	snmp->addr[3] = addr[3];
	snmp->port = port;
	snmp->sock = -1;
	snmp->id = NULL;
	snmp->index.len = 5;
	snmp->index.subs[0] = addr[0];
	snmp->index.subs[1] = addr[1];
	snmp->index.subs[2] = addr[2];
	snmp->index.subs[3] = addr[3];
	snmp->index.subs[4] = port;

	/*
	 * Insert it into the right place
	 */
	TAILQ_FOREACH(p, &snmp_port_list, link) {
		if (asn_compare_oid(&p->index, &snmp->index) > 0) {
			TAILQ_INSERT_BEFORE(p, snmp, link);
			break;
		}
	}
	if (p == NULL)
		TAILQ_INSERT_TAIL(&snmp_port_list, snmp, link);

	if (community != COMM_INITIALIZE &&
	    (err = init_snmp(snmp)) != SNMP_ERR_NOERROR) {
		TAILQ_REMOVE(&snmp_port_list, snmp, link);
		free(snmp);
		return (err);
	}
	*pp = snmp;
	return (SNMP_ERR_NOERROR);
}

/*
 * Close an SNMP port
 */
void
close_snmp_port(struct snmp_port *snmp)
{
	if (snmp->id != NULL)
		fd_deselect(snmp->id);
	if (snmp->sock >= 0)
		(void)close(snmp->sock);

	TAILQ_REMOVE(&snmp_port_list, snmp, link);
	free(snmp);
}

/*
 * Create a local socket
 */
static int
init_local(struct local_port *p)
{
	struct sockaddr_un sa;

	if ((p->sock = socket(PF_LOCAL, SOCK_DGRAM, 0)) < 0) {
		syslog(LOG_ERR, "creating local socket: %m");
		return (SNMP_ERR_RES_UNAVAIL);
	}
	strcpy(sa.sun_path, p->name);
	sa.sun_family = AF_LOCAL;
	sa.sun_len = strlen(p->name) + offsetof(struct sockaddr_un, sun_path);

	(void)remove(p->name);

	if (bind(p->sock, (struct sockaddr *)&sa, sizeof(sa))) {
		if (errno == EADDRNOTAVAIL) {
			close(p->sock);
			p->sock = -1;
			return (SNMP_ERR_INCONS_NAME);
		}
		syslog(LOG_ERR, "bind: %s %m", p->name);
		close(p->sock);
		p->sock = -1;
		return (SNMP_ERR_GENERR);
	}
	if (chmod(p->name, 0666) == -1)
		syslog(LOG_WARNING, "chmod(%s,0666): %m", p->name);
	if ((p->id = fd_select(p->sock, lsock_input, p, NULL)) == NULL) {
		(void)remove(p->name);
		close(p->sock);
		p->sock = -1;
		return (SNMP_ERR_GENERR);
	}
	return (SNMP_ERR_NOERROR);
}


/*
 * Open a local port
 */
int
open_local_port(u_char *name, size_t namelen, struct local_port **pp)
{
	struct local_port *port, *p;
	size_t u;
	int err;
	struct sockaddr_un sa;

	if (namelen == 0 || namelen + 1 > sizeof(sa.sun_path)) {
		free(name);
		return (SNMP_ERR_BADVALUE);
	}
	if ((port = malloc(sizeof(*port))) == NULL) {
		free(name);
		return (SNMP_ERR_GENERR);
	}
	if ((port->name = malloc(namelen + 1)) == NULL) {
		free(name);
		free(port);
		return (SNMP_ERR_GENERR);
	}
	strncpy(port->name, name, namelen);
	port->name[namelen] = '\0';

	port->sock = -1;
	port->id = NULL;
	port->index.len = namelen + 1;
	port->index.subs[0] = namelen;
	for (u = 0; u < namelen; u++)
		port->index.subs[u + 1] = name[u];

	/*
	 * Insert it into the right place
	 */
	TAILQ_FOREACH(p, &local_port_list, link) {
		if (asn_compare_oid(&p->index, &port->index) > 0) {
			TAILQ_INSERT_BEFORE(p, port, link);
			break;
		}
	}
	if (p == NULL)
		TAILQ_INSERT_TAIL(&local_port_list, port, link);

	if (community != COMM_INITIALIZE &&
	    (err = init_local(port)) != SNMP_ERR_NOERROR) {
		TAILQ_REMOVE(&local_port_list, port, link);
		free(port->name);
		free(port);
		return (err);
	}

	*pp = p;

	return (SNMP_ERR_NOERROR);
}

/*
 * Close a local port
 */
void
close_local_port(struct local_port *port)
{
	if (port->id != NULL)
		fd_deselect(port->id);
	if (port->sock >= 0)
		(void)close(port->sock);
	(void)remove(port->name);

	TAILQ_REMOVE(&local_port_list, port, link);
	free(port->name);
	free(port);
}

/*
 * Dump internal state.
 */
static void
info_func(evContext ctx __unused, void *uap __unused, const void *tag __unused)
{
	struct lmodule *m;
	u_int i;
	char buf[10000];

	syslog(LOG_DEBUG, "Dump of SNMPd %lu\n", (u_long)getpid());
	for (i = 0; i < tree_size; i++) {
		switch (tree[i].type) {

		  case SNMP_NODE_LEAF:
			sprintf(buf, "LEAF: %s %s", tree[i].name,
			    asn_oid2str(&tree[i].oid));
			break;

		  case SNMP_NODE_COLUMN:
			sprintf(buf, "COL: %s %s", tree[i].name,
			    asn_oid2str(&tree[i].oid));
			break;
		}
		syslog(LOG_DEBUG, "%s", buf);
	}

	TAILQ_FOREACH(m, &lmodules, link)
		if (m->config->dump)
			(*m->config->dump)();
}

/*
 * Re-read configuration
 */
static void
config_func(evContext ctx __unused, void *uap __unused,
    const void *tag __unused)
{
	struct lmodule *m;

	if (read_config(config_file, NULL)) {
		syslog(LOG_ERR, "error reading config file '%s'", config_file);
		return;
	}
	TAILQ_FOREACH(m, &lmodules, link)
		if (m->config->config)
			(*m->config->config)();
}

/*
 * On USR1 dump actual configuration.
 */
static void
onusr1(int s __unused)
{
	work |= WORK_DOINFO;
}
static void
onhup(int s __unused)
{
	work |= WORK_RECONFIG;
}

static void
onterm(int s __unused)
{
	struct local_port *p;

	TAILQ_FOREACH(p, &local_port_list, link)
		(void)remove(p->name);

	exit(0);
}


static void
init_sigs(void)
{
	struct sigaction sa;

	sa.sa_handler = onusr1;
	sa.sa_flags = SA_RESTART;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL)) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(1);
	}

	sa.sa_handler = onhup;
	if (sigaction(SIGHUP, &sa, NULL)) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(1);
	}

	sa.sa_handler = onterm;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGTERM, &sa, NULL)) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(1);
	}
	if (sigaction(SIGINT, &sa, NULL)) {
		syslog(LOG_ERR, "sigaction: %m");
		exit(1);
	}
}

static void
block_sigs(void)
{
	sigset_t set;

	sigfillset(&set);
	if (sigprocmask(SIG_BLOCK, &set, &blocked_sigs) == -1) {
		syslog(LOG_ERR, "SIG_BLOCK: %m");
		exit(1);
	}
}
static void
unblock_sigs(void)
{
	if (sigprocmask(SIG_SETMASK, &blocked_sigs, NULL) == -1) {
		syslog(LOG_ERR, "SIG_SETMASK: %m");
		exit(1);
	}
}

/*
 * Shut down
 */
static void
term(void)
{
	(void)unlink(pid_file);
}

/*
 * Define a macro from the command line
 */
static void
do_macro(char *arg)
{
	char *eq;
	int err;

	if ((eq = strchr(arg, '=')) == NULL)
		err = define_macro(arg, "");
	else {
		*eq++ = '\0';
		err = define_macro(arg, eq);
	}
	if (err == -1) {
		syslog(LOG_ERR, "cannot save macro: %m");
		exit(1);
	}
}

/*
 * Re-implement getsubopt from scratch, because the second argument is broken
 * and will not compile with WARNS=5.
 */
static int
getsubopt1(char **arg, const char *const *options, char **valp, char **optp)
{
	static const char *const delim = ",\t ";
	u_int i;
	char *ptr;

	*optp = NULL;

	/* skip leading junk */
	for (ptr = *arg; *ptr != '\0'; ptr++)
		if (strchr(delim, *ptr) == NULL)
			break;
	if (*ptr == '\0') {
		*arg = ptr;
		return (-1);
	}
	*optp = ptr;

	/* find the end of the option */
	while (*++ptr != '\0')
		if (strchr(delim, *ptr) != NULL || *ptr == '=')
			break;

	if (*ptr != '\0') {
		if (*ptr == '=') {
			*ptr++ = '\0';
			*valp = ptr;
			while (*ptr != '\0' && strchr(delim, *ptr) == NULL)
				ptr++;
			if (*ptr != '\0')
				*ptr++ = '\0';
		} else
			*ptr++ = '\0';
	}

	*arg = ptr;

	for (i = 0; *options != NULL; options++, i++)
		if (strcmp(suboptarg, *options) == 0)
			return (i);
	return (-1);
}

int
main(int argc, char *argv[])
{
	int opt;
	FILE *fp;
	int background = 1;
	struct snmp_port *p;
	struct local_port *pl;
	const char *prefix = "snmpd";
	struct lmodule *m;
	char *value, *option;

#define DBG_DUMP	0
#define DBG_EVENTS	1
#define DBG_TRACE	2
	static const char *const debug_opts[] = {
		"dump",
		"events",
		"trace",
		NULL
	};

	snmp_printf = snmp_printf_func;
	snmp_error = snmp_error_func;
	snmp_debug = snmp_debug_func;
	asn_error = asn_error_func;

	while ((opt = getopt(argc, argv, "c:dD:hI:l:m:p:")) != EOF)
		switch (opt) {

		  case 'c':
			strlcpy(config_file, optarg, sizeof(config_file));
			break;

		  case 'd':
			background = 0;
			break;

		  case 'D':
			while (*optarg) {
				switch (getsubopt1(&optarg, debug_opts,
				    &value, &option)) {

				  case DBG_DUMP:
					debug.dump_pdus = 1;
					break;

				  case DBG_EVENTS:
					debug.evdebug++;
					break;

				  case DBG_TRACE:
					if (value == NULL)
						syslog(LOG_ERR,
						    "no value for 'trace'");
					snmp_trace = strtoul(value, NULL, 0);
					break;

				  case -1:
					if (suboptarg)
						syslog(LOG_ERR,
						    "unknown debug flag '%s'",
						    option);
					else
						syslog(LOG_ERR,
						    "missing debug flag");
					break;
				}
			}
			break;

		  case 'h':
			fprintf(stderr, "%s", usgtxt);
			exit(0);

		  case 'I':
			syspath = optarg;
			break;

		  case 'l':
			prefix = optarg;
			break;

		  case 'm':
			do_macro(optarg);
			break;

		  case 'p':
			strlcpy(pid_file, optarg, sizeof(pid_file));
			break;
		}

	openlog(prefix, LOG_PID | (background ? 0 : LOG_PERROR), LOG_USER);
	setlogmask(LOG_UPTO(debug.logpri - 1));

	if (background && daemon(0, 0) < 0) {
		syslog(LOG_ERR, "daemon: %m");
		exit(1);
	}

	argc -= optind;
	argv += optind;

	progargs = argv;
	nprogargs = argc;

	srandomdev();

	snmp_serial_no = random();

	/*
	 * Initialize the tree.
	 */
	if ((tree = malloc(sizeof(struct snmp_node) * CTREE_SIZE)) == NULL) {
		syslog(LOG_ERR, "%m");
		exit(1);
	}
	memcpy(tree, ctree, sizeof(struct snmp_node) * CTREE_SIZE);
	tree_size = CTREE_SIZE;

	/*
	 * Get standard communities
	 */
	(void)comm_define(1, "SNMP read", NULL, "public");
	(void)comm_define(2, "SNMP write", NULL, "public");
	community = COMM_INITIALIZE;

	trap_reqid = reqid_allocate(512, NULL);

	if (config_file[0] == '\0')
		snprintf(config_file, sizeof(config_file), PATH_CONFIG, prefix);

	init_actvals();
	if (read_config(config_file, NULL)) {
		syslog(LOG_ERR, "error in config file");
		exit(1);
	}

	if (evCreate(&evctx)) {
		syslog(LOG_ERR, "evCreate: %m");
		exit(1);
	}
	if (debug.evdebug > 0)
		evSetDebug(evctx, 10, stderr);

	TAILQ_FOREACH(p, &snmp_port_list, link)
		(void)init_snmp(p);
	TAILQ_FOREACH(pl, &local_port_list, link)
		(void)init_local(pl);

	init_sigs();

	if (pid_file[0] == '\0')
		snprintf(pid_file, sizeof(pid_file), PATH_PID, prefix);

	if ((fp = fopen(pid_file, "w")) != NULL) {
		fprintf(fp, "%u", getpid());
		fclose(fp);
		atexit(term);
	}

	start_tick = get_ticks();
	this_tick = get_ticks();

	if (or_register(&oid_snmpMIB, "The MIB module for SNMPv2 entities.",
	    NULL) == 0) {
		syslog(LOG_ERR, "cannot register SNMPv2 MIB");
		exit(1);
	}
	if (or_register(&oid_begemotSnmpd, "The MIB module for the Begemot SNMPd.",
	    NULL) == 0) {
		syslog(LOG_ERR, "cannot register begemotSnmpd MIB");
		exit(1);
	}

	snmp_send_trap(&oid_coldStart, NULL);

	while ((m = TAILQ_FIRST(&modules_start)) != NULL) {
		m->flags &= ~LM_ONSTARTLIST;
		TAILQ_REMOVE(&modules_start, m, start);
		lm_start(m);
	}

	for (;;) {
		evEvent event;
		struct lmodule *mod;

		TAILQ_FOREACH(mod, &lmodules, link)
			if (mod->config->idle != NULL)
				(*mod->config->idle)();

		if (evGetNext(evctx, &event, EV_WAIT) == 0) {
			if (evDispatch(evctx, event))
				syslog(LOG_ERR, "evDispatch: %m");
		} else if (errno != EINTR) {
			syslog(LOG_ERR, "evGetNext: %m");
			exit(1);
		}

		if (work != 0) {
			block_sigs();
			if (work & WORK_DOINFO) {
				if (evWaitFor(evctx, &work, info_func,
				    NULL, NULL) == -1) {
					syslog(LOG_ERR, "evWaitFor: %m");
					exit(1);
				}
			}
			if (work & WORK_RECONFIG) {
				if (evWaitFor(evctx, &work, config_func,
				    NULL, NULL) == -1) {
					syslog(LOG_ERR, "evWaitFor: %m");
					exit(1);
				}
			}
			work = 0;
			unblock_sigs();
			if (evDo(evctx, &work) == -1) {
				syslog(LOG_ERR, "evDo: %m");
				exit(1);
			}
		}
	}

	return (0);
}


u_int32_t
get_ticks()
{
	struct timeval tv;
	u_int32_t ret;

	if (gettimeofday(&tv, NULL))
		abort();
	ret = tv.tv_sec * 100 + tv.tv_usec / 10000;
	return (ret);
}
/*
 * Timer support
 */
static void
tfunc(evContext ctx __unused, void *uap, struct timespec due __unused,
	struct timespec inter __unused)
{
	struct timer *tp = uap;

	LIST_REMOVE(tp, link);
	tp->func(tp->udata);
	free(tp);
}

/*
 * Start a timer
 */
void *
timer_start(u_int ticks, void (*func)(void *), void *udata, struct lmodule *mod)
{
	struct timer *tp;
	struct timespec due;

	if ((tp = malloc(sizeof(struct timer))) == NULL) {
		syslog(LOG_CRIT, "out of memory for timer");
		exit(1);
	}
	due = evAddTime(evNowTime(),
			evConsTime(ticks / 100, (ticks % 100) * 10000));

	tp->udata = udata;
	tp->owner = mod;
	tp->func = func;

	LIST_INSERT_HEAD(&timer_list, tp, link);

	if (evSetTimer(evctx, tfunc, tp, due, evConsTime(0, 0), &tp->id)
	    == -1) {
		syslog(LOG_ERR, "cannot set timer: %m");
		exit(1);
	}
	return (tp);
}

void
timer_stop(void *p)
{
	struct timer *tp = p;

	LIST_REMOVE(tp, link);
	if (evClearTimer(evctx, tp->id) == -1) {
		syslog(LOG_ERR, "cannot stop timer: %m");
		exit(1);
	}
	free(p);
}

static void
timer_flush(struct lmodule *mod)
{
	struct timer *t, *t1;

	t = LIST_FIRST(&timer_list);
	while (t != NULL) {
		t1 = LIST_NEXT(t, link);
		if (t->owner == mod)
			timer_stop(t);
		t = t1;
	}
}

static void
snmp_printf_func(const char *fmt, ...)
{
	va_list ap;
	static char *pend = NULL;
	char *ret, *new;

	va_start(ap, fmt);
	vasprintf(&ret, fmt, ap);
	va_end(ap);

	if (ret == NULL)
		return;
	if (pend != NULL) {
		if ((new = realloc(pend, strlen(pend) + strlen(ret) + 1))
		    == NULL) {
			free(ret);
			return;
		}
		pend = new;
		strcat(pend, ret);
		free(ret);
	} else
		pend = ret;

	while ((ret = strchr(pend, '\n')) != NULL) {
		*ret = '\0';
		syslog(LOG_DEBUG, "%s", pend);
		if (strlen(ret + 1) == 0) {
			free(pend);
			pend = NULL;
			break;
		}
		strcpy(pend, ret + 1);
	}
}

static void
snmp_error_func(const char *err, ...)
{
	char errbuf[1000];
	va_list ap;

	va_start(ap, err);
	snprintf(errbuf, sizeof(errbuf), "SNMP: ");
	vsnprintf(errbuf+strlen(errbuf), sizeof(errbuf)-strlen(errbuf),
	    err, ap);
	va_end(ap);

	syslog(LOG_ERR, "%s", errbuf);
}

static void
snmp_debug_func(const char *err, ...)
{
	char errbuf[1000];
	va_list ap;

	va_start(ap, err);
	snprintf(errbuf, sizeof(errbuf), "SNMP: ");
	vsnprintf(errbuf+strlen(errbuf), sizeof(errbuf)-strlen(errbuf),
	    err, ap);
	va_end(ap);

	syslog(LOG_DEBUG, "%s", errbuf);
}

static void
asn_error_func(const struct asn_buf *b, const char *err, ...)
{
	char errbuf[1000];
	va_list ap;
	u_int i;

	va_start(ap, err);
	snprintf(errbuf, sizeof(errbuf), "ASN.1: ");
	vsnprintf(errbuf+strlen(errbuf), sizeof(errbuf)-strlen(errbuf),
	    err, ap);
	va_end(ap);

	if (b != NULL) {
		snprintf(errbuf+strlen(errbuf), sizeof(errbuf)-strlen(errbuf),
		    " at");
		for (i = 0; b->asn_len > i; i++)
			snprintf(errbuf+strlen(errbuf),
			    sizeof(errbuf)-strlen(errbuf), " %02x", b->asn_cptr[i]);
	}

	syslog(LOG_ERR, "%s", errbuf);
}

/*
 * Create a new community
 */
u_int
comm_define(u_int priv, const char *descr, struct lmodule *owner,
    const char *str)
{
	struct community *c, *p;
	u_int ncomm;

	/* generate an identifier */
	do {
		if ((ncomm = next_community_index++) == UINT_MAX)
			next_community_index = 1;
		TAILQ_FOREACH(c, &community_list, link)
			if (c->value == ncomm)
				break;
	} while (c != NULL);

	if ((c = malloc(sizeof(struct community))) == NULL) {
		syslog(LOG_ERR, "comm_define: %m");
		return (0);
	}
	c->owner = owner;
	c->value = ncomm;
	c->descr = descr;
	c->string = NULL;
	c->private = priv;

	if (str != NULL) {
		if((c->string = malloc(strlen(str)+1)) == NULL) {
			free(c);
			return (0);
		}
		strcpy(c->string, str);
	}

	/* make index */
	if (c->owner == NULL) {
		c->index.len = 1;
		c->index.subs[0] = 0;
	} else {
		c->index = c->owner->index;
	}
	c->index.subs[c->index.len++] = c->private;

	/*
	 * Insert ordered
	 */
	TAILQ_FOREACH(p, &community_list, link) {
		if (asn_compare_oid(&p->index, &c->index) > 0) {
			TAILQ_INSERT_BEFORE(p, c, link);
			break;
		}
	}
	if (p == NULL)
		TAILQ_INSERT_TAIL(&community_list, c, link);
	return (c->value);
}

const char *
comm_string(u_int ncomm)
{
	struct community *p;

	TAILQ_FOREACH(p, &community_list, link)
		if (p->value == ncomm)
			return (p->string);
	return (NULL);
}

/*
 * Delete all communities allocated by a module
 */
static void
comm_flush(struct lmodule *mod)
{
	struct community *p, *p1;

	p = TAILQ_FIRST(&community_list);
	while (p != NULL) {
		p1 = TAILQ_NEXT(p, link);
		if (p->owner == mod) {
			free(p->string);
			TAILQ_REMOVE(&community_list, p, link);
			free(p);
		}
		p = p1;
	}
}

/*
 * Request ID handling.
 *
 * Allocate a new range of request ids. Use a first fit algorithm.
 */
u_int
reqid_allocate(int size, struct lmodule *mod)
{
	u_int type;
	struct idrange *r, *r1;

	if (size <= 0 || size > INT32_MAX) {
		syslog(LOG_CRIT, "%s: size out of range: %d", __func__, size);
		return (0);
	}
	/* allocate a type id */
	do {
		if ((type = next_idrange++) == UINT_MAX)
			next_idrange = 1;
		TAILQ_FOREACH(r, &idrange_list, link)
			if (r->type == type)
				break;
	} while(r != NULL);

	/* find a range */
	if (TAILQ_EMPTY(&idrange_list))
		r = NULL;
	else {
		r = TAILQ_FIRST(&idrange_list);
		if (r->base < size) {
			while((r1 = TAILQ_NEXT(r, link)) != NULL) {
				if (r1->base - (r->base + r->size) >= size)
					break;
				r = r1;
			}
			r = r1;
		}
		if (r == NULL) {
			r1 = TAILQ_LAST(&idrange_list, idrange_list);
			if (INT32_MAX - size + 1 < r1->base + r1->size) {
				syslog(LOG_ERR, "out of id ranges (%u)", size);
				return (0);
			}
		}
	}

	/* allocate structure */
	if ((r1 = malloc(sizeof(struct idrange))) == NULL) {
		syslog(LOG_ERR, "%s: %m", __FUNCTION__);
		return (0);
	}

	r1->type = type;
	r1->size = size;
	r1->owner = mod;
	if (TAILQ_EMPTY(&idrange_list) || r == TAILQ_FIRST(&idrange_list)) {
		r1->base = 0;
		TAILQ_INSERT_HEAD(&idrange_list, r1, link);
	} else if (r == NULL) {
		r = TAILQ_LAST(&idrange_list, idrange_list);
		r1->base = r->base + r->size;
		TAILQ_INSERT_TAIL(&idrange_list, r1, link);
	} else {
		r = TAILQ_PREV(r, idrange_list, link);
		r1->base = r->base + r->size;
		TAILQ_INSERT_AFTER(&idrange_list, r, r1, link);
	}
	r1->next = r1->base;

	return (type);
}

int32_t
reqid_next(u_int type)
{
	struct idrange *r;
	int32_t id;

	TAILQ_FOREACH(r, &idrange_list, link)
		if (r->type == type)
			break;
	if (r == NULL) {
		syslog(LOG_CRIT, "wrong idrange type");
		abort();
	}
	if ((id = r->next++) == r->base + (r->size - 1))
		r->next = r->base;
	return (id);
}

int32_t
reqid_base(u_int type)
{
	struct idrange *r;

	TAILQ_FOREACH(r, &idrange_list, link)
		if (r->type == type)
			return (r->base);
	syslog(LOG_CRIT, "wrong idrange type");
	abort();
}

u_int
reqid_type(int32_t reqid)
{
	struct idrange *r;

	TAILQ_FOREACH(r, &idrange_list, link)
		if (reqid >= r->base && reqid <= r->base + (r->size - 1))
			return (r->type);
	return (0);
}

int
reqid_istype(int32_t reqid, u_int type)
{
	return (reqid_type(reqid) == type);
}

/*
 * Delete all communities allocated by a module
 */
static void
reqid_flush(struct lmodule *mod)
{
	struct idrange *p, *p1;

	p = TAILQ_FIRST(&idrange_list);
	while (p != NULL) {
		p1 = TAILQ_NEXT(p, link);
		if (p->owner == mod) {
			TAILQ_REMOVE(&idrange_list, p, link);
			free(p);
		}
		p = p1;
	}
}

/*
 * Merge the given tree for the given module into the main tree.
 */
static int
compare_node(const void *v1, const void *v2)
{
	const struct snmp_node *n1 = v1;
	const struct snmp_node *n2 = v2;

	return (asn_compare_oid(&n1->oid, &n2->oid));
}
static int
tree_merge(const struct snmp_node *ntree, u_int nsize, struct lmodule *mod)
{
	struct snmp_node *xtree;
	u_int i;

	xtree = realloc(tree, sizeof(*tree) * (tree_size + nsize));
	if (xtree == NULL) {
		syslog(LOG_ERR, "lm_load: %m");
		return (-1);
	}
	tree = xtree;
	memcpy(&tree[tree_size], ntree, sizeof(*tree) * nsize);

	for (i = 0; i < nsize; i++)
		tree[tree_size + i].data = mod;

	tree_size += nsize;

	qsort(tree, tree_size, sizeof(tree[0]), compare_node);

	return (0);
}

/*
 * Remove all nodes belonging to the loadable module
 */
static void
tree_unmerge(struct lmodule *mod)
{
	u_int s, d;

	for(s = d = 0; s < tree_size; s++)
		if (tree[s].data != mod) {
			if (s != d)
				tree[d] = tree[s];
			d++;
		}
	tree_size = d;
}

/*
 * Loadable modules
 */
struct lmodule *
lm_load(const char *path, const char *section)
{
	struct lmodule *m;
	int err;
	int i;
	char *av[MAX_MOD_ARGS + 1];
	int ac;
	u_int u;

	if ((m = malloc(sizeof(*m))) == NULL) {
		syslog(LOG_ERR, "lm_load: %m");
		return (NULL);
	}
	m->handle = NULL;
	m->flags = 0;
	strcpy(m->section, section);

	if ((m->path = malloc(strlen(path) + 1)) == NULL) {
		syslog(LOG_ERR, "lm_load: %m");
		goto err;
	}
	strcpy(m->path, path);

	/*
	 * Make index
	 */
	m->index.subs[0] = strlen(section);
	m->index.len = m->index.subs[0] + 1;
	for (u = 0; u < m->index.subs[0]; u++)
		m->index.subs[u + 1] = section[u];

	/*
	 * Load the object file and locate the config structure
	 */
	if ((m->handle = dlopen(m->path, RTLD_NOW|RTLD_GLOBAL)) == NULL) {
		syslog(LOG_ERR, "lm_load: open %s", dlerror());
		goto err;
	}

	if ((m->config = dlsym(m->handle, "config")) == NULL) {
		syslog(LOG_ERR, "lm_load: no 'config' symbol %s", dlerror());
		goto err;
	}

	/*
	 * Insert it into the right place
	 */
	INSERT_OBJECT_OID(m, &lmodules);

	/* preserve order */
	if (community == COMM_INITIALIZE) {
		m->flags |= LM_ONSTARTLIST;
		TAILQ_INSERT_TAIL(&modules_start, m, start);
	}

	/*
	 * make the argument vector.
	 */
	ac = 0;
	for (i = 0; i < nprogargs; i++) {
		if (strlen(progargs[i]) >= strlen(section) + 1 &&
		    strncmp(progargs[i], section, strlen(section)) == 0 &&
		    progargs[i][strlen(section)] == ':') {
			if (ac == MAX_MOD_ARGS) {
				syslog(LOG_WARNING, "too many arguments for "
				    "module '%s", section);
				break;
			}
			av[ac++] = &progargs[i][strlen(section)+1];
		}
	}
	av[ac] = NULL;

	/*
	 * Run the initialisation function
	 */
	if ((err = (*m->config->init)(m, ac, av)) != 0) {
		syslog(LOG_ERR, "lm_load: init failed: %d", err);
		TAILQ_REMOVE(&lmodules, m, link);
		goto err;
	}

	return (m);

  err:
	if (m->handle)
		dlclose(m->handle);
	free(m->path);
	free(m);
	return (NULL);
}

/*
 * Start a module
 */
void
lm_start(struct lmodule *mod)
{
	const struct lmodule *m;

	/*
	 * Merge tree. If this fails, unload the module.
	 */
	if (tree_merge(mod->config->tree, mod->config->tree_size, mod)) {
		lm_unload(mod);
		return;
	}

	/*
	 * Read configuration
	 */
	if (read_config(config_file, mod)) {
		syslog(LOG_ERR, "error in config file");
		lm_unload(mod);
		return;
	}
	if (mod->config->start)
		(*mod->config->start)();

	mod->flags |= LM_STARTED;

	/*
	 * Inform other modules
	 */
	TAILQ_FOREACH(m, &lmodules, link)
		if (m->config->loading)
			(*m->config->loading)(mod, 1);
}


/*
 * Unload a module.
 */
void
lm_unload(struct lmodule *m)
{
	int err;
	const struct lmodule *mod;

	TAILQ_REMOVE(&lmodules, m, link);
	if (m->flags & LM_ONSTARTLIST)
		TAILQ_REMOVE(&modules_start, m, start);
	tree_unmerge(m);

	if ((m->flags & LM_STARTED) && m->config->fini &&
	    (err = (*m->config->fini)()) != 0)
		syslog(LOG_WARNING, "lm_unload(%s): fini %d", m->section, err);

	comm_flush(m);
	reqid_flush(m);
	timer_flush(m);
	fd_flush(m);

	dlclose(m->handle);
	free(m->path);

	/*
	 * Inform other modules
	 */
	TAILQ_FOREACH(mod, &lmodules, link)
		if (mod->config->loading)
			(*mod->config->loading)(m, 0);

	free(m);
}

/*
 * Register an object resource and return the index (or 0 on failures)
 */
u_int
or_register(const struct asn_oid *or, const char *descr, struct lmodule *mod)
{
	struct objres *objres, *or1;
	u_int idx;

	/* find a free index */
	idx = 1;
	for (objres = TAILQ_FIRST(&objres_list);
	     objres != NULL;
	     objres = TAILQ_NEXT(objres, link)) {
		if ((or1 = TAILQ_NEXT(objres, link)) == NULL ||
		    or1->index > objres->index + 1) {
			idx = objres->index + 1;
			break;
		}
	}

	if ((objres = malloc(sizeof(*objres))) == NULL)
		return (0);

	objres->index = idx;
	objres->oid = *or;
	strlcpy(objres->descr, descr, sizeof(objres->descr));
	objres->uptime = get_ticks() - start_tick;
	objres->module = mod;

	INSERT_OBJECT_INT(objres, &objres_list);

	systemg.or_last_change = objres->uptime;

	return (idx);
}

void
or_unregister(u_int idx)
{
	struct objres *objres;

	TAILQ_FOREACH(objres, &objres_list, link)
		if (objres->index == idx) {
			TAILQ_REMOVE(&objres_list, objres, link);
			free(objres);
			return;
		}
}
