/*-
 *
 * Copyright (c) 1999-2001, Vitaly V Belekhov
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */


#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/mbuf.h>
#include <sys/errno.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/syslog.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/netisr.h>

#include <netinet/in.h>
#include <netinet/if_ether.h>

#include <netgraph/ng_message.h>
#include <netgraph/netgraph.h>
#include <netgraph/ng_parse.h>
#include <netgraph/ng_eiface.h>

#include <net/bpf.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

static const struct ng_parse_struct_field ng_eiface_par_fields[]
	= NG_EIFACE_PAR_FIELDS;

static const struct ng_parse_type ng_eiface_par_type = {
	&ng_parse_struct_type,
	&ng_eiface_par_fields
};

static const struct ng_cmdlist ng_eiface_cmdlist[] = {
	{
	  NGM_EIFACE_COOKIE,
	  NGM_EIFACE_SET,
	  "set",
	  &ng_eiface_par_type,
	  NULL
	},
	{ 0 }
};


/* Node private data */
struct ng_eiface_private {
	struct ifnet   *ifp;	/* This interface */
	int	unit;		/* Interface unit number */
	struct arpcom   arpcom;	/* per-interface network data */
	node_p		node;	/* Our netgraph node */
	hook_p		ether;	/* Hook for ethernet stream */
};
typedef struct ng_eiface_private *priv_p;

/* Interface methods */
static void     ng_eiface_init(void *xsc);
static void     ng_eiface_start(struct ifnet *ifp);
static int      ng_eiface_ioctl(struct ifnet *ifp, u_long cmd, caddr_t data);
#ifdef DEBUG
static void     ng_eiface_print_ioctl(struct ifnet *ifp, int cmd, caddr_t data);
#endif

/* Netgraph methods */
static ng_constructor_t ng_eiface_constructor;
static ng_rcvmsg_t ng_eiface_rcvmsg;
static ng_shutdown_t ng_eiface_rmnode;
static ng_newhook_t ng_eiface_newhook;
static ng_rcvdata_t ng_eiface_rcvdata;
static ng_connect_t ng_eiface_connect;
static ng_disconnect_t ng_eiface_disconnect;

/* Node type descriptor */
static struct ng_type typestruct = {
	NG_ABI_VERSION,
	NG_EIFACE_NODE_TYPE,
	NULL,
	ng_eiface_constructor,
	ng_eiface_rcvmsg,
	ng_eiface_rmnode,
	ng_eiface_newhook,
	NULL,
	ng_eiface_connect,
	ng_eiface_rcvdata,
	ng_eiface_disconnect,
	ng_eiface_cmdlist
};
NETGRAPH_INIT(eiface, &typestruct);

/* We keep a bitmap indicating which unit numbers are free.
   One means the unit number is free, zero means it's taken. */
static int	*ng_eiface_units = NULL;
static int	ng_eiface_units_len = 0;
static int	ng_units_in_use = 0;

#define UNITS_BITSPERWORD	(sizeof(*ng_eiface_units) * NBBY)


/************************************************************************
			HELPER STUFF
 ************************************************************************/
/*
 * Find the first free unit number for a new interface.
 * Increase the size of the unit bitmap as necessary.
 */
static __inline__ int
ng_eiface_get_unit(int *unit)
{
	int index, bit;

	for (index = 0; index < ng_eiface_units_len
	    && ng_eiface_units[index] == 0; index++);
	if (index == ng_eiface_units_len) {		/* extend array */
		int i, *newarray, newlen;

		newlen = (2 * ng_eiface_units_len) + 4;
		MALLOC(newarray, int *, newlen * sizeof(*ng_eiface_units),
		    M_NETGRAPH, M_NOWAIT);
		if (newarray == NULL)
			return (ENOMEM);
		bcopy(ng_eiface_units, newarray,
		    ng_eiface_units_len * sizeof(*ng_eiface_units));
		for (i = ng_eiface_units_len; i < newlen; i++)
			newarray[i] = ~0;
		if (ng_eiface_units != NULL)
			FREE(ng_eiface_units, M_NETGRAPH);
		ng_eiface_units = newarray;
		ng_eiface_units_len = newlen;
	}
	bit = ffs(ng_eiface_units[index]) - 1;
	KASSERT(bit >= 0 && bit <= UNITS_BITSPERWORD - 1,
	    ("%s: word=%d bit=%d", __func__, ng_eiface_units[index], bit));
	ng_eiface_units[index] &= ~(1 << bit);
	*unit = (index * UNITS_BITSPERWORD) + bit;
	ng_units_in_use++;
	return (0);
}

/*
 * Free a no longer needed unit number.
 */
static __inline__ void
ng_eiface_free_unit(int unit)
{
	int index, bit;

	index = unit / UNITS_BITSPERWORD;
	bit = unit % UNITS_BITSPERWORD;
	KASSERT(index < ng_eiface_units_len,
	    ("%s: unit=%d len=%d", __func__, unit, ng_eiface_units_len));
	KASSERT((ng_eiface_units[index] & (1 << bit)) == 0,
	    ("%s: unit=%d is free", __func__, unit));
	ng_eiface_units[index] |= (1 << bit);
	/*
	 * XXX We could think about reducing the size of ng_eiface_units[]
	 * XXX here if the last portion is all ones
	 * XXX At least free it if no more units.
	 * Needed if we are to eventually be able to unload.
	 */
	ng_units_in_use--;
	if (ng_units_in_use == 0) { /* XXX make SMP safe */
		FREE(ng_eiface_units, M_NETGRAPH);
		ng_eiface_units_len = 0;
		ng_eiface_units = NULL;
	}
}

/************************************************************************
			INTERFACE STUFF
 ************************************************************************/

/*
 * Process an ioctl for the virtual interface
 */
static int
ng_eiface_ioctl(struct ifnet *ifp, u_long command, caddr_t data)
{
	struct ifreq   *const ifr = (struct ifreq *)data;
	int		s, error = 0;

#ifdef DEBUG
	ng_eiface_print_ioctl(ifp, command, data);
#endif
	s = splimp();
	switch (command)
	{
	/* These two are mostly handled at a higher layer */
	case SIOCSIFADDR:
		error = ether_ioctl(ifp, command, data);
		break;
	case SIOCGIFADDR:
		break;

	/* Set flags */
	case SIOCSIFFLAGS:
		/*
		 * If the interface is marked up and stopped, then
		 * start it. If it is marked down and running,
		 * then stop it.
		 */
		if (ifr->ifr_flags & IFF_UP) {
			if (!(ifp->if_flags & IFF_RUNNING)) {
				ifp->if_flags &= ~(IFF_OACTIVE);
				ifp->if_flags |= IFF_RUNNING;
			}
		} else {
			if (ifp->if_flags & IFF_RUNNING)
				ifp->if_flags
					&= ~(IFF_RUNNING | IFF_OACTIVE);
		}
		break;

	/* Set the interface MTU */
	case SIOCSIFMTU:
		if (ifr->ifr_mtu > NG_EIFACE_MTU_MAX
		|| ifr->ifr_mtu < NG_EIFACE_MTU_MIN)
			error = EINVAL;
		else
			ifp->if_mtu = ifr->ifr_mtu;
		break;

	/* Stuff that's not supported */
	case SIOCADDMULTI:
	case SIOCDELMULTI:
		error = 0;
		break;
	case SIOCSIFPHYS:
		error = EOPNOTSUPP;
		break;

	default:
		error = EINVAL;
		break;
	}
	(void)splx(s);
	return (error);
}

static void
ng_eiface_init(void *xsc)
{
	priv_p		sc = xsc;
	struct ifnet   *ifp = sc->ifp;
	int		s;

	s = splimp();

	ifp->if_flags |= IFF_RUNNING;
	ifp->if_flags &= ~IFF_OACTIVE;

	splx(s);
}

/*
 * We simply relay the packet to the ether hook, if it is connected.
 * We have been throughthe netgraph locking an are guaranteed to 
 * be the only code running in this node at this time.
 */
static void
ng_eiface_start2(node_p node, hook_p hook, void *arg1, int arg2)
{
	struct ifnet *ifp = arg1;
	const priv_p priv = (priv_p) ifp->if_softc;
	int		len, error = 0;
	struct mbuf    *m;

	/* Check interface flags */
	if ((ifp->if_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING))
		return;

	/* Don't do anything if output is active */
	if (ifp->if_flags & IFF_OACTIVE)
		return;

	ifp->if_flags |= IFF_OACTIVE;

	/*
	 * Grab a packet to transmit.
	 */
	IF_DEQUEUE(&ifp->if_snd, m);

	/* If there's nothing to send, return. */
	if (m == NULL) {
		ifp->if_flags &= ~IFF_OACTIVE;
		return;
	}

	/* Berkeley packet filter
	 * Pass packet to bpf if there is a listener.
	 * XXX is this safe? locking?
	 */
	BPF_MTAP(ifp, m);

	/* Copy length before the mbuf gets invalidated */
	len = m->m_pkthdr.len;

	/*
	 * Send packet; if hook is not connected, mbuf will get
	 * freed.
	 */
	NG_SEND_DATA_ONLY(error, priv->ether, m);

	/* Update stats */
	if (error == 0) {
		ifp->if_obytes += len;
		ifp->if_opackets++;
	}
	ifp->if_flags &= ~IFF_OACTIVE;
	return;
}

/*
 * This routine is called to deliver a packet out the interface.
 * We simply queue the netgraph version to be called when netgraph locking
 * allows it to happen.
 * Until we know what the rest of the networking code is doing for
 * locking, we don't know how we will interact with it.
 * Take comfort from the fact that the ifnet struct is part of our
 * private info and can't go away while we are queued.
 * [Though we don't know it is still there now....]
 * it is possible we don't gain anything from this because
 * we would like to get the mbuf and queue it as data
 * somehow, but we can't and if we did would we solve anything?
 */
static void
ng_eiface_start(struct ifnet *ifp)
{
	
	const priv_p priv = (priv_p) ifp->if_softc;

	ng_send_fn(priv->node, NULL, &ng_eiface_start2, ifp, 0);
}

#ifdef DEBUG
/*
 * Display an ioctl to the virtual interface
 */

static void
ng_eiface_print_ioctl(struct ifnet *ifp, int command, caddr_t data){
	char		*str;

	switch (command & IOC_DIRMASK)
	{
	case IOC_VOID:
		str = "IO";
		break;
	case IOC_OUT:
		str = "IOR";
		break;
	case IOC_IN:
		str = "IOW";
		break;
	case IOC_INOUT:
		str = "IORW";
		break;
	default:
		str = "IO??";
	}
	log(LOG_DEBUG, "%s: %s('%c', %d, char[%d])\n",
			ifp->if_xname,
			str,
			IOCGROUP(command),
			command & 0xff,
			IOCPARM_LEN(command));
}
#endif	/* DEBUG */

/************************************************************************
			NETGRAPH NODE STUFF
 ************************************************************************/

/*
 * Constructor for a node
 */
static int
ng_eiface_constructor(node_p node)
{
	struct ifnet   *ifp;
	priv_p		priv;
	int		error = 0;

	/* Allocate node and interface private structures */
	MALLOC(priv, priv_p, sizeof(*priv), M_NETGRAPH, M_WAITOK);
	if (priv == NULL) {
		return (ENOMEM);
	}
	bzero(priv, sizeof(*priv));

	ifp = &(priv->arpcom.ac_if);

	/* Link them together */
	ifp->if_softc = priv;
	priv->ifp = ifp;

	/* Get an interface unit number */
	if ((error = ng_eiface_get_unit(&priv->unit)) != 0) {
		FREE(priv, M_NETGRAPH);
		return (error);
	}

	/* Link together node and private info */
	NG_NODE_SET_PRIVATE(node, priv);
	priv->node = node;

	/* Initialize interface structure */
	if_initname(ifp, NG_EIFACE_EIFACE_NAME, priv->unit);
	ifp->if_init = ng_eiface_init;
	ifp->if_output = ether_output;
	ifp->if_start = ng_eiface_start;
	ifp->if_ioctl = ng_eiface_ioctl;
	ifp->if_watchdog = NULL;
	ifp->if_snd.ifq_maxlen = IFQ_MAXLEN;
	ifp->if_flags = (IFF_SIMPLEX | IFF_BROADCAST | IFF_MULTICAST);

	/*
	 * Give this node name * bzero(ifname, sizeof(ifname));
	 * sprintf(ifname, "if%s", ifp->if_xname); (void)
	 * ng_name_node(node, ifname);
	 */

	/* Attach the interface */
	ether_ifattach(ifp, priv->arpcom.ac_enaddr);

	/* Done */
	return (0);
}

/*
 * Give our ok for a hook to be added
 */
static int
ng_eiface_newhook(node_p node, hook_p hook, const char *name)
{
	priv_p		priv = NG_NODE_PRIVATE(node);

	if (strcmp(name, NG_EIFACE_HOOK_ETHER))
		return (EPFNOSUPPORT);
	if (priv->ether != NULL)
		return (EISCONN);
	priv->ether = hook;
	NG_HOOK_SET_PRIVATE(hook, &priv->ether);

	return (0);
}

/*
 * Receive a control message
 */
static int
ng_eiface_rcvmsg(node_p node, item_p item, hook_p lasthook)
{
	priv_p		priv = NG_NODE_PRIVATE(node);
	struct ifnet   *const ifp = priv->ifp;
	struct ng_mesg *resp = NULL;
	int		error = 0;
	struct ng_mesg *msg;

	NGI_GET_MSG(item, msg);
	switch		(msg->header.typecookie) {
	case NGM_EIFACE_COOKIE:
		switch (msg->header.cmd) {
		case NGM_EIFACE_SET:
		{
			struct ng_eiface_par *eaddr;
			struct ifaddr *ifa;
			struct sockaddr_dl *sdl;

			if (msg->header.arglen != sizeof(struct ng_eiface_par)){
				error = EINVAL;
				break;
			}
			eaddr = (struct ng_eiface_par *)(msg->data);

			priv->arpcom.ac_enaddr[0] = eaddr->oct0;
			priv->arpcom.ac_enaddr[1] = eaddr->oct1;
			priv->arpcom.ac_enaddr[2] = eaddr->oct2;
			priv->arpcom.ac_enaddr[3] = eaddr->oct3;
			priv->arpcom.ac_enaddr[4] = eaddr->oct4;
			priv->arpcom.ac_enaddr[5] = eaddr->oct5;

			/* And put it in the ifaddr list */
#define IFP2AC(IFP) ((struct arpcom *)IFP)
			TAILQ_FOREACH(ifa, &(ifp->if_addrhead), ifa_link) {
				sdl = (struct sockaddr_dl *)ifa->ifa_addr;
				if (sdl->sdl_type == IFT_ETHER) {
					bcopy((IFP2AC(ifp))->ac_enaddr,
						LLADDR(sdl), ifp->if_addrlen);
					break;
				}
			}
			break;
		}

		case NGM_EIFACE_GET_IFNAME:
		{
			struct ng_eiface_ifname *arg;

			NG_MKRESPONSE(resp, msg, sizeof(*arg), M_NOWAIT);
			if (resp == NULL) {
				error = ENOMEM;
				break;
			}
			arg = (struct ng_eiface_ifname *)resp->data;
			strlcpy(arg->ngif_name, ifp->if_xname,
			    sizeof(arg->ngif_name));
			break;
		}

		case NGM_EIFACE_GET_IFADDRS:
		{
			struct ifaddr  *ifa;
			caddr_t		ptr;
			int		buflen;

#define SA_SIZE(s)	((s)->sa_len<sizeof(*(s))? sizeof(*(s)):(s)->sa_len)

			/* Determine size of response and allocate it */
			buflen = 0;
			TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link)
			buflen += SA_SIZE(ifa->ifa_addr);
			NG_MKRESPONSE(resp, msg, buflen, M_NOWAIT);
			if (resp == NULL) {
				error = ENOMEM;
				break;
			}
			/* Add addresses */
			ptr = resp->data;
			TAILQ_FOREACH(ifa, &ifp->if_addrhead, ifa_link) {
				const int       len = SA_SIZE(ifa->ifa_addr);

				if (buflen < len) {
					log(LOG_ERR, "%s: len changed?\n",
					ifp->if_xname);
					break;
				}
				bcopy(ifa->ifa_addr, ptr, len);
				ptr += len;
				buflen -= len;
			}
			break;
#undef SA_SIZE
		}

		default:
			error = EINVAL;
			break;
		} /* end of inner switch() */
		break;
	default:
		error = EINVAL;
		break;
	}
	NG_RESPOND_MSG(error, node, item, resp);
	NG_FREE_MSG(msg);
	return (error);
}

/*
 * Recive data from a hook. Pass the packet to the ether_input routine.
 */
static int
ng_eiface_rcvdata(hook_p hook, item_p item)
{
	priv_p		priv = NG_NODE_PRIVATE(NG_HOOK_NODE(hook));
	struct ifnet   *const ifp = priv->ifp;
	struct mbuf *m;

	NGI_GET_M(item, m);
	/* Meta-data ends its life here... */
        NG_FREE_ITEM(item);

	if (m == NULL)
	{
		printf("ng_eiface: mbuf is null.\n");
		return (EINVAL);
	}
	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING)) {
		NG_FREE_M(m);
		return (ENETDOWN);
	}

	/* Note receiving interface */
	m->m_pkthdr.rcvif = ifp;

	/* Update interface stats */
	ifp->if_ipackets++;

	(*ifp->if_input)(ifp, m);

	/* Done */
	return (0);
}

/*
 * the node.
 */
static int
ng_eiface_rmnode(node_p node)
{
	priv_p		priv = NG_NODE_PRIVATE(node);
	struct ifnet   *const ifp = priv->ifp;

	ether_ifdetach(ifp);
	ng_eiface_free_unit(priv->unit);
	FREE(priv, M_NETGRAPH);
	NG_NODE_SET_PRIVATE(node, NULL);
	NG_NODE_UNREF(node);
	return (0);
}


/*
 * This is called once we've already connected a new hook to the other node.
 * It gives us a chance to balk at the last minute.
 */
static int
ng_eiface_connect(hook_p hook)
{
	/* be really amiable and just say "YUP that's OK by me! " */
	return (0);
}

/*
 * Hook disconnection
 */
static int
ng_eiface_disconnect(hook_p hook)
{
	priv_p		priv = NG_NODE_PRIVATE(NG_HOOK_NODE(hook));

	priv->ether = NULL;
	return (0);
}
