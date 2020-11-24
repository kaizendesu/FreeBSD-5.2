/*-
 * Copyright (c) 2003 Poul-Henning Kamp
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <err.h>
#include <sys/disk.h>

static void
usage(void)
{
	fprintf(stderr, "Usage: diskinfo [-tv]\n");
	exit (1);
}

static int opt_t, opt_v;

static void speeddisk(const char *name, int fd, off_t mediasize, u_int sectorsize);

int
main(int argc, char **argv)
{
	int i, ch, fd, error;
	char buf[BUFSIZ];
	off_t	mediasize;
	u_int	sectorsize, fwsectors, fwheads;

	while ((ch = getopt(argc, argv, "tv")) != -1) {
		switch (ch) {
		case 't':
			opt_t = 1;
			opt_v = 1;
			break;
		case 'v':
			opt_v = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	for (i = 0; i < argc; i++) {
		fd = open(argv[i], O_RDONLY);
		if (fd < 0 && errno == ENOENT && *argv[i] != '/') {
			sprintf(buf, "%s%s", _PATH_DEV, argv[i]);
			fd = open(buf, O_RDONLY);
		}
		if (fd < 0)
			err(1, argv[i]);
		error = ioctl(fd, DIOCGMEDIASIZE, &mediasize);
		if (error)
			err(1, "%s: ioctl(DIOCGMEDIASIZE) failed, probably not a disk.", argv[i]);
		error = ioctl(fd, DIOCGSECTORSIZE, &sectorsize);
		if (error)
			err(1, "%s: DIOCGSECTORSIZE failed, probably not a disk.", argv[i]);
		error = ioctl(fd, DIOCGFWSECTORS, &fwsectors);
		if (error)
			fwsectors = 0;
		error = ioctl(fd, DIOCGFWHEADS, &fwheads);
		if (error)
			fwheads = 0;
		if (!opt_v) {
			printf("%s", argv[i]);
			printf("\t%u", sectorsize);
			printf("\t%jd", (intmax_t)mediasize);
			printf("\t%jd", (intmax_t)mediasize/sectorsize);
			if (fwsectors != 0 && fwheads != 0) {
				printf("\t%jd", (intmax_t)mediasize /
				    (fwsectors * fwheads * sectorsize));
				printf("\t%u", fwheads);
				printf("\t%u", fwsectors);
			} 
		} else {
			printf("%s\n", argv[i]);
			printf("\t%-12u\t# sectorsize\n", sectorsize);
			printf("\t%-12jd\t# mediasize in bytes\n",
			    (intmax_t)mediasize);
			printf("\t%-12jd\t# mediasize in sectors\n",
			    (intmax_t)mediasize/sectorsize);
			if (fwsectors != 0 && fwheads != 0) {
				printf("\t%-12jd\t# Cylinders according to firmware.\n", (intmax_t)mediasize /
				    (fwsectors * fwheads * sectorsize));
				printf("\t%-12u\t# Heads according to firmware.\n", fwheads);
				printf("\t%-12u\t# Sectors according to firmware.\n", fwsectors);
			} 
		}
		printf("\n");
		if (opt_t)
			speeddisk(argv[i], fd, mediasize, sectorsize);
		close(fd);
	}
	exit (0);
}


static char sector[65536];
static char mega[1024 * 1024];

static void
rdsect(int fd, u_int blockno, u_int sectorsize)
{
	int error;

	lseek(fd, (off_t)blockno * sectorsize, SEEK_SET);
	error = read(fd, sector, sectorsize);
	if (error != sectorsize)
		err(1, "read error or disk too small for test.");
}

static void
rdmega(int fd)
{
	int error;

	error = read(fd, mega, sizeof(mega));
	if (error != sizeof(mega))
		err(1, "read error or disk too small for test.");
}

static struct timeval tv1, tv2;

static void
T0(void)
{

	fflush(stdout);
	sync();
	sleep(1);
	sync();
	sync();
	gettimeofday(&tv1, NULL);
}

static void
TN(int count)
{
	double dt;

	gettimeofday(&tv2, NULL);
	dt = (tv2.tv_usec - tv1.tv_usec) / 1e6;
	dt += (tv2.tv_sec - tv1.tv_sec);
	printf("%5d iter in %10.6f sec = %8.3f msec\n",
		count, dt, dt * 1000.0 / count);
}

static void
TR(double count)
{
	double dt;

	gettimeofday(&tv2, NULL);
	dt = (tv2.tv_usec - tv1.tv_usec) / 1e6;
	dt += (tv2.tv_sec - tv1.tv_sec);
	printf("%8.0f kbytes in %10.6f sec = %8.0f kbytes/sec\n",
		count, dt, count / dt);
}

static void
speeddisk(const char *name, int fd, off_t mediasize, u_int sectorsize)
{
	int error, i;
	uint b0, b1, sectorcount;

	off_t size;
	sectorcount = mediasize / sectorsize;

	printf("Seek times:\n");
	printf("\tFull stroke:\t");
	b0 = 0;
	b1 = sectorcount - 1 - 16384;
	T0();
	for (i = 0; i < 125; i++) {
		rdsect(fd, b0, sectorsize);
		b0 += 16384;
		rdsect(fd, b1, sectorsize);
		b1 -= 16384;
	}
	TN(250);

	printf("\tHalf stroke:\t");
	b0 = sectorcount / 4;
	b1 = b0 + sectorcount / 2;
	T0();
	for (i = 0; i < 125; i++) {
		rdsect(fd, b0, sectorsize);
		b0 += 16384;
		rdsect(fd, b1, sectorsize);
		b1 += 16384;
	}
	TN(250);
	printf("\tQuarter stroke:\t");
	b0 = sectorcount / 4;
	b1 = b0 + sectorcount / 4;
	T0();
	for (i = 0; i < 250; i++) {
		rdsect(fd, b0, sectorsize);
		b0 += 16384;
		rdsect(fd, b1, sectorsize);
		b1 += 16384;
	}
	TN(500);

	printf("\tShort forward:\t");
	b0 = sectorcount / 2;
	T0();
	for (i = 0; i < 400; i++) {
		rdsect(fd, b0, sectorsize);
		b0 += 16384;
	}
	TN(400);

	printf("\tShort backward:\t");
	b0 = sectorcount / 2;
	T0();
	for (i = 0; i < 400; i++) {
		rdsect(fd, b0, sectorsize);
		b0 -= 16384;
	}
	TN(400);

	printf("\tSeq outer:\t");
	b0 = 0;
	T0();
	for (i = 0; i < 2048; i++) {
		rdsect(fd, b0, sectorsize);
		b0++;
	}
	TN(2048);

	printf("\tSeq inner:\t");
	b0 = sectorcount - 2048 - 1;
	T0();
	for (i = 0; i < 2048; i++) {
		rdsect(fd, b0, sectorsize);
		b0++;
	}
	TN(2048);

	printf("Transfer rates:\n");
	printf("\toutside:     ");
	rdsect(fd, 0, sectorsize);
	T0();
	for (i = 0; i < 100; i++) {
		rdmega(fd);
	}
	TR(100 * 1024);

	printf("\tmiddle:      ");
	b0 = sectorcount / 2;
	rdsect(fd, b0, sectorsize);
	T0();
	for (i = 0; i < 100; i++) {
		rdmega(fd);
	}
	TR(100 * 1024);

	printf("\tinside:      ");
	b0 = sectorcount - 100 * (1024*1024 / sectorsize) - 1;;
	rdsect(fd, b0, sectorsize);
	T0();
	for (i = 0; i < 100; i++) {
		rdmega(fd);
	}
	TR(100 * 1024);

	printf("\n");

	return;
}