# $FreeBSD$

LIB=	ssh
SRCS=	authfd.c authfile.c bufaux.c buffer.c canohost.c channels.c \
	cipher.c compat.c compress.c crc32.c deattack.c fatal.c \
	hostfile.c log.c match.c mpaux.c nchan.c packet.c readpass.c \
	rsa.c tildexpand.c ttymodes.c xmalloc.c atomicio.c \
	key.c dispatch.c kex.c mac.c uuencode.c misc.c \
	rijndael.c ssh-dss.c ssh-rsa.c dh.c kexdh.c kexgex.c \
	kexdhc.c kexgexc.c scard.c msg.c progressmeter.c \
	entropy.c
# Portability layer
SRCS+=	bsd-misc.c vis.c xmmap.c
# FreeBSD additions
SRCS+=	version.c

.if defined(COMPAT_GETADDRINFO)
SRCS+=	getaddrinfo.c getnameinfo.c name6.c rcmd.c bindresvport.c
.endif

CFLAGS+= -I${SSHDIR}
.if !defined(NO_KERBEROS)
CFLAGS+= -DKRB5 -DHEIMDAL
.endif

NOLINT=	true

DPADD=	${LIBCRYPTO} ${LIBZ}
LDADD=	-lcrypto -lz

.include <bsd.lib.mk>

.PATH: ${SSHDIR} ${SSHDIR}/openbsd-compat
