/* $xMach: strnsubst.c,v 1.3 2002/02/23 02:10:24 jmallett Exp $ */

/*
 * Copyright (c) 2002 J. Mallett.  All rights reserved.
 * You may do whatever you want with this file as long as
 * the above copyright and this notice remain intact, along
 * with the following statement:
 * 	For the man who taught me vi, and who got too old, too young.
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

#include <err.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void	strnsubst(char **, const char *, const char *, size_t);

/*
 * Replaces str with a string consisting of str with match replaced with
 * replstr as many times as can be done before the constructed string is
 * maxsize bytes large.  It does not free the string pointed to by str, it
 * is up to the calling program to be sure that the original contents of
 * str as well as the new contents are handled in an appropriate manner.
 * If replstr is NULL, then that internally is changed to a nil-string, so
 * that we can still pretend to do somewhat meaningful substitution.
 * No value is returned.
 */
void
strnsubst(char **str, const char *match, const char *replstr, size_t maxsize)
{
	char *s1, *s2, *this;

	s1 = *str;
	if (s1 == NULL)
		return;
	s2 = calloc(maxsize, 1);
	if (s2 == NULL)
		err(1, "calloc");

	if (replstr == NULL)
		replstr = "";

	if (match == NULL || replstr == NULL || maxsize == strlen(s1)) {
		strlcpy(s2, s1, maxsize);
		goto done;
	}

	for (;;) {
		this = strstr(s1, match);
		if (this == NULL)
			break;
		if ((strlen(s2) + ((uintptr_t)this - (uintptr_t)s1) +
		    (strlen(replstr) - 1)) > maxsize && *replstr != '\0') {
			strlcat(s2, s1, maxsize);
			goto done;
		}
		strncat(s2, s1, (uintptr_t)this - (uintptr_t)s1);
		strcat(s2, replstr);
		s1 = this + strlen(match);
	}
	strcat(s2, s1);
done:
	*str = s2;
	return;
}

#ifdef TEST
#include <stdio.h>

int 
main(void)
{
	char *x, *y, *z, *za;

	x = "{}%$";
	strnsubst(&x, "%$", "{} enpury!", 255);
	y = x;
	strnsubst(&y, "}{}", "ybir", 255);
	z = y;
	strnsubst(&z, "{", "v ", 255);
	za = z;
	strnsubst(&z, NULL, za, 255);
	if (strcmp(z, "v ybir enpury!") == 0)
		printf("strnsubst() seems to work!\n");
	else
		printf("strnsubst() is broken.\n");
	printf("%s\n", z);
	free(x);
	free(y);
	free(z);
	free(za);
	return 0;
}
#endif
