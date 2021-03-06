/*
 * Derived from:
 *
 * MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/*
 *  Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
 *  rights reserved.
 *
 *  RSA Data Security, Inc. makes no representations concerning either
 *  the merchantability of this software or the suitability of this
 *  software for any particular purpose. It is provided "as is"
 *  without express or implied warranty of any kind.
 *
 *  These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <err.h>
#include <md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * Length of test block, number of test blocks.
 */
#define TEST_BLOCK_LEN 10000
#define TEST_BLOCK_COUNT 100000

int qflag;
int rflag;
int sflag;

static void MDString(const char *);
static void MDTimeTrial(void);
static void MDTestSuite(void);
static void MDFilter(int);
static void usage(void);

/* Main driver.

Arguments (may be any combination):
  -sstring - digests string
  -t       - runs time trial
  -x       - runs test script
  filename - digests file
  (none)   - digests standard input
 */
int
main(int argc, char *argv[])
{
	int     ch;
	char   *p;
	char	buf[33];
	int     failed;

	failed = 0;
	while ((ch = getopt(argc, argv, "pqrs:tx")) != -1)
		switch (ch) {
		case 'p':
			MDFilter(1);
			break;
		case 'q':
			qflag = 1;
			break;
		case 'r':
			rflag = 1;
			break;
		case 's':
			sflag = 1;
			MDString(optarg);
			break;
		case 't':
			MDTimeTrial();
			break;
		case 'x':
			MDTestSuite();
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (*argv) {
		do {
			p = MD5File(*argv, buf);
			if (!p) {
				warn("%s", *argv);
				failed++;
			} else {
				if (qflag)
					printf("%s\n", p);
				else if (rflag)
					printf("%s %s\n", p, *argv);
				else
					printf("MD5 (%s) = %s\n", *argv, p);
			}
		} while (*++argv);
	} else if (!sflag && (optind == 1 || qflag || rflag))
		MDFilter(0);

	if (failed != 0)
		return (1);
 
	return (0);
}
/*
 * Digests a string and prints the result.
 */
static void
MDString(const char *string)
{
	size_t len = strlen(string);
	char buf[33];

	if (qflag)
		printf("%s\n", MD5Data(string, len, buf));
	else if (rflag)
		printf("%s \"%s\"\n", MD5Data(string, len, buf), string);
	else
		printf("MD5 (\"%s\") = %s\n", string, MD5Data(string, len, buf));
}
/*
 * Measures the time to digest TEST_BLOCK_COUNT TEST_BLOCK_LEN-byte blocks.
 */
static void
MDTimeTrial(void)
{
	MD5_CTX context;
	struct rusage before, after;
	struct timeval total;
	float seconds;
	unsigned char block[TEST_BLOCK_LEN];
	unsigned int i;
	char   *p, buf[33];

	printf
	    ("MD5 time trial. Digesting %d %d-byte blocks ...",
	    TEST_BLOCK_COUNT, TEST_BLOCK_LEN);
	fflush(stdout);

	/* Initialize block */
	for (i = 0; i < TEST_BLOCK_LEN; i++)
		block[i] = (unsigned char) (i & 0xff);

	/* Start timer */
	getrusage(0, &before);

	/* Digest blocks */
	MD5Init(&context);
	for (i = 0; i < TEST_BLOCK_COUNT; i++)
		MD5Update(&context, block, TEST_BLOCK_LEN);
	p = MD5End(&context,buf);

	/* Stop timer */
	getrusage(0, &after);
	timersub(&after.ru_utime, &before.ru_utime, &total);
	seconds = total.tv_sec + (float) total.tv_usec / 1000000;

	printf(" done\n");
	printf("Digest = %s", p);
	printf("\nTime = %f seconds\n", seconds);
	printf
	    ("Speed = %f bytes/second\n",
	    (float) TEST_BLOCK_LEN * (float) TEST_BLOCK_COUNT / seconds);
}
/*
 * Digests a reference suite of strings and prints the results.
 */

#define MD5TESTCOUNT 8

char *MDTestInput[] = {
	"", 
	"a",
	"abc",
	"message digest",
	"abcdefghijklmnopqrstuvwxyz",
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
	"12345678901234567890123456789012345678901234567890123456789012345678901234567890",
	"MD5 has not yet (2001-09-03) been broken, but sufficient attacks have been made that its security is in some doubt"
};

char *MDTestOutput[MD5TESTCOUNT] = {
	"d41d8cd98f00b204e9800998ecf8427e",
	"0cc175b9c0f1b6a831c399e269772661",
	"900150983cd24fb0d6963f7d28e17f72",
	"f96b697d7cb7938d525a2f31aaf161d0",
	"c3fcd3d76192e4007dfb496cca67e13b",
	"d174ab98d277d9f5a5611c2c9f419d9f",
	"57edf4a22be3c955ac49da2e2107b67a",
	"b50663f41d44d92171cb9976bc118538"
};

static void
MDTestSuite(void)
{
	int i;
	char buffer[33];

	printf("MD5 test suite:\n");
	for (i = 0; i < MD5TESTCOUNT; i++) {
		MD5Data(MDTestInput[i], strlen(MDTestInput[i]), buffer);
		printf("MD5 (\"%s\") = %s", MDTestInput[i], buffer);
		if (strcmp(buffer, MDTestOutput[i]) == 0)
			printf(" - verified correct\n");
		else
			printf(" - INCORRECT RESULT!\n");
	}
}

/*
 * Digests the standard input and prints the result.
 */
static void
MDFilter(int tee)
{
	MD5_CTX context;
	unsigned int len;
	unsigned char buffer[BUFSIZ];
	char buf[33];

	MD5Init(&context);
	while ((len = fread(buffer, 1, BUFSIZ, stdin))) {
		if (tee && len != fwrite(buffer, 1, len, stdout))
			err(1, "stdout");
		MD5Update(&context, buffer, len);
	}
	printf("%s\n", MD5End(&context,buf));
}

static void
usage(void)
{

	fprintf(stderr, "usage: md5 [-pqrtx] [-s string] [files ...]\n");
	exit(1);
}
