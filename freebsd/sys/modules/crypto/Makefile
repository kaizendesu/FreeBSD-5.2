# $FreeBSD$

.PATH:	${.CURDIR}/../../opencrypto
.PATH:	${.CURDIR}/../../crypto
.PATH:	${.CURDIR}/../../crypto/blowfish
.PATH:	${.CURDIR}/../../crypto/des
.PATH:	${.CURDIR}/../../crypto/sha2
.PATH:	${.CURDIR}/../../net

KMOD	= crypto
SRCS	= crypto.c
SRCS	+= criov.c crmbuf.c cryptosoft.c xform.c
SRCS	+= cast.c deflate.c rmd160.c rijndael.c skipjack.c
SRCS	+= bf_enc.c bf_skey.c
SRCS	+= des_ecb.c des_enc.c des_setkey.c
SRCS	+= sha1.c sha2.c
SRCS	+= zlib.c
SRCS	+= opt_param.h

.include <bsd.kmod.mk>
