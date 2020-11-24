/*
 * System call switch table.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * $FreeBSD$
 * created from FreeBSD: src/sys/compat/freebsd32/syscalls.master,v 1.26.2.1 2003/12/13 22:18:27 peter Exp 
 */

#include "opt_compat.h"

#include <sys/param.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/mount.h>
#include <compat/freebsd32/freebsd32.h>
#include <compat/freebsd32/freebsd32_proto.h>

#define AS(name) (sizeof(struct name) / sizeof(register_t))

#ifdef COMPAT_FREEBSD4
#define compat4(n, name) n, (sy_call_t *)__CONCAT(freebsd4_,name)
#else
#define compat4(n, name) 0, (sy_call_t *)nosys
#endif

/* The casts are bogus but will do for now. */
struct sysent freebsd32_sysent[] = {
	{ SYF_MPSAFE | 0, (sy_call_t *)nosys },		/* 0 = syscall */
	{ SYF_MPSAFE | AS(sys_exit_args), (sy_call_t *)sys_exit },	/* 1 = exit */
	{ SYF_MPSAFE | 0, (sy_call_t *)fork },		/* 2 = fork */
	{ SYF_MPSAFE | AS(read_args), (sy_call_t *)read },	/* 3 = read */
	{ SYF_MPSAFE | AS(write_args), (sy_call_t *)write },	/* 4 = write */
	{ AS(freebsd32_open_args), (sy_call_t *)freebsd32_open },	/* 5 = freebsd32_open */
	{ SYF_MPSAFE | AS(close_args), (sy_call_t *)close },	/* 6 = close */
	{ SYF_MPSAFE | AS(freebsd32_wait4_args), (sy_call_t *)freebsd32_wait4 },	/* 7 = freebsd32_wait4 */
	{ 0, (sy_call_t *)nosys },			/* 8 = obsolete old creat */
	{ AS(link_args), (sy_call_t *)link },		/* 9 = link */
	{ AS(unlink_args), (sy_call_t *)unlink },	/* 10 = unlink */
	{ 0, (sy_call_t *)nosys },			/* 11 = obsolete execv */
	{ AS(chdir_args), (sy_call_t *)chdir },		/* 12 = chdir */
	{ AS(fchdir_args), (sy_call_t *)fchdir },	/* 13 = fchdir */
	{ AS(mknod_args), (sy_call_t *)mknod },		/* 14 = mknod */
	{ AS(chmod_args), (sy_call_t *)chmod },		/* 15 = chmod */
	{ AS(chown_args), (sy_call_t *)chown },		/* 16 = chown */
	{ SYF_MPSAFE | AS(obreak_args), (sy_call_t *)obreak },	/* 17 = break */
	{ AS(freebsd32_getfsstat_args), (sy_call_t *)freebsd32_getfsstat },	/* 18 = freebsd32_getfsstat */
	{ 0, (sy_call_t *)nosys },			/* 19 = obsolete olseek */
	{ SYF_MPSAFE | 0, (sy_call_t *)getpid },	/* 20 = getpid */
	{ AS(mount_args), (sy_call_t *)mount },		/* 21 = mount */
	{ AS(unmount_args), (sy_call_t *)unmount },	/* 22 = unmount */
	{ SYF_MPSAFE | AS(setuid_args), (sy_call_t *)setuid },	/* 23 = setuid */
	{ SYF_MPSAFE | 0, (sy_call_t *)getuid },	/* 24 = getuid */
	{ SYF_MPSAFE | 0, (sy_call_t *)geteuid },	/* 25 = geteuid */
	{ SYF_MPSAFE | AS(ptrace_args), (sy_call_t *)ptrace },	/* 26 = ptrace */
	{ 0, (sy_call_t *)nosys },			/* 27 = recvmsg */
	{ SYF_MPSAFE | AS(sendmsg_args), (sy_call_t *)sendmsg },	/* 28 = sendmsg */
	{ SYF_MPSAFE | AS(recvfrom_args), (sy_call_t *)recvfrom },	/* 29 = recvfrom */
	{ SYF_MPSAFE | AS(accept_args), (sy_call_t *)accept },	/* 30 = accept */
	{ SYF_MPSAFE | AS(getpeername_args), (sy_call_t *)getpeername },	/* 31 = getpeername */
	{ SYF_MPSAFE | AS(getsockname_args), (sy_call_t *)getsockname },	/* 32 = getsockname */
	{ AS(freebsd32_access_args), (sy_call_t *)freebsd32_access },	/* 33 = freebsd32_access */
	{ AS(freebsd32_chflags_args), (sy_call_t *)freebsd32_chflags },	/* 34 = freebsd32_chflags */
	{ AS(fchflags_args), (sy_call_t *)fchflags },	/* 35 = fchflags */
	{ 0, (sy_call_t *)sync },			/* 36 = sync */
	{ SYF_MPSAFE | AS(kill_args), (sy_call_t *)kill },	/* 37 = kill */
	{ 0, (sy_call_t *)nosys },			/* 38 = ostat */
	{ SYF_MPSAFE | 0, (sy_call_t *)getppid },	/* 39 = getppid */
	{ 0, (sy_call_t *)nosys },			/* 40 = olstat */
	{ SYF_MPSAFE | AS(dup_args), (sy_call_t *)dup },	/* 41 = dup */
	{ SYF_MPSAFE | 0, (sy_call_t *)pipe },		/* 42 = pipe */
	{ SYF_MPSAFE | 0, (sy_call_t *)getegid },	/* 43 = getegid */
	{ SYF_MPSAFE | AS(profil_args), (sy_call_t *)profil },	/* 44 = profil */
	{ SYF_MPSAFE | AS(ktrace_args), (sy_call_t *)ktrace },	/* 45 = ktrace */
	{ 0, (sy_call_t *)nosys },			/* 46 = osigaction */
	{ SYF_MPSAFE | 0, (sy_call_t *)getgid },	/* 47 = getgid */
	{ 0, (sy_call_t *)nosys },			/* 48 = osigprocmask */
	{ SYF_MPSAFE | AS(getlogin_args), (sy_call_t *)getlogin },	/* 49 = getlogin */
	{ SYF_MPSAFE | AS(setlogin_args), (sy_call_t *)setlogin },	/* 50 = setlogin */
	{ SYF_MPSAFE | AS(acct_args), (sy_call_t *)acct },	/* 51 = acct */
	{ 0, (sy_call_t *)nosys },			/* 52 = obsolete osigpending */
	{ SYF_MPSAFE | AS(freebsd32_sigaltstack_args), (sy_call_t *)freebsd32_sigaltstack },	/* 53 = freebsd32_sigaltstack */
	{ SYF_MPSAFE | AS(ioctl_args), (sy_call_t *)ioctl },	/* 54 = ioctl */
	{ SYF_MPSAFE | AS(reboot_args), (sy_call_t *)reboot },	/* 55 = reboot */
	{ AS(revoke_args), (sy_call_t *)revoke },	/* 56 = revoke */
	{ AS(symlink_args), (sy_call_t *)symlink },	/* 57 = symlink */
	{ AS(readlink_args), (sy_call_t *)readlink },	/* 58 = readlink */
	{ AS(freebsd32_execve_args), (sy_call_t *)freebsd32_execve },	/* 59 = freebsd32_execve */
	{ SYF_MPSAFE | AS(umask_args), (sy_call_t *)umask },	/* 60 = umask */
	{ AS(chroot_args), (sy_call_t *)chroot },	/* 61 = chroot */
	{ 0, (sy_call_t *)nosys },			/* 62 = obsolete ofstat */
	{ 0, (sy_call_t *)nosys },			/* 63 = obsolete ogetkerninfo */
	{ 0, (sy_call_t *)nosys },			/* 64 = obsolete ogetpagesize */
	{ 0, (sy_call_t *)nosys },			/* 65 = obsolete omsync */
	{ SYF_MPSAFE | 0, (sy_call_t *)vfork },		/* 66 = vfork */
	{ 0, (sy_call_t *)nosys },			/* 67 = obsolete vread */
	{ 0, (sy_call_t *)nosys },			/* 68 = obsolete vwrite */
	{ SYF_MPSAFE | AS(sbrk_args), (sy_call_t *)sbrk },	/* 69 = sbrk */
	{ SYF_MPSAFE | AS(sstk_args), (sy_call_t *)sstk },	/* 70 = sstk */
	{ 0, (sy_call_t *)nosys },			/* 71 = obsolete ommap */
	{ SYF_MPSAFE | AS(ovadvise_args), (sy_call_t *)ovadvise },	/* 72 = vadvise */
	{ SYF_MPSAFE | AS(munmap_args), (sy_call_t *)munmap },	/* 73 = munmap */
	{ SYF_MPSAFE | AS(mprotect_args), (sy_call_t *)mprotect },	/* 74 = mprotect */
	{ SYF_MPSAFE | AS(madvise_args), (sy_call_t *)madvise },	/* 75 = madvise */
	{ 0, (sy_call_t *)nosys },			/* 76 = obsolete vhangup */
	{ 0, (sy_call_t *)nosys },			/* 77 = obsolete vlimit */
	{ SYF_MPSAFE | AS(mincore_args), (sy_call_t *)mincore },	/* 78 = mincore */
	{ SYF_MPSAFE | AS(getgroups_args), (sy_call_t *)getgroups },	/* 79 = getgroups */
	{ SYF_MPSAFE | AS(setgroups_args), (sy_call_t *)setgroups },	/* 80 = setgroups */
	{ SYF_MPSAFE | 0, (sy_call_t *)getpgrp },	/* 81 = getpgrp */
	{ SYF_MPSAFE | AS(setpgid_args), (sy_call_t *)setpgid },	/* 82 = setpgid */
	{ AS(freebsd32_setitimer_args), (sy_call_t *)freebsd32_setitimer },	/* 83 = freebsd32_setitimer */
	{ 0, (sy_call_t *)nosys },			/* 84 = obsolete owait */
	{ 0, (sy_call_t *)nosys },			/* 85 = obsolete oswapon */
	{ 0, (sy_call_t *)nosys },			/* 86 = obsolete ogetitimer */
	{ 0, (sy_call_t *)nosys },			/* 87 = obsolete ogethostname */
	{ 0, (sy_call_t *)nosys },			/* 88 = obsolete osethostname */
	{ SYF_MPSAFE | 0, (sy_call_t *)getdtablesize },	/* 89 = getdtablesize */
	{ SYF_MPSAFE | AS(dup2_args), (sy_call_t *)dup2 },	/* 90 = dup2 */
	{ 0, (sy_call_t *)nosys },			/* 91 = getdopt */
	{ SYF_MPSAFE | AS(fcntl_args), (sy_call_t *)fcntl },	/* 92 = fcntl */
	{ AS(freebsd32_select_args), (sy_call_t *)freebsd32_select },	/* 93 = freebsd32_select */
	{ 0, (sy_call_t *)nosys },			/* 94 = setdopt */
	{ AS(fsync_args), (sy_call_t *)fsync },		/* 95 = fsync */
	{ SYF_MPSAFE | AS(setpriority_args), (sy_call_t *)setpriority },	/* 96 = setpriority */
	{ SYF_MPSAFE | AS(socket_args), (sy_call_t *)socket },	/* 97 = socket */
	{ SYF_MPSAFE | AS(connect_args), (sy_call_t *)connect },	/* 98 = connect */
	{ 0, (sy_call_t *)nosys },			/* 99 = obsolete oaccept */
	{ SYF_MPSAFE | AS(getpriority_args), (sy_call_t *)getpriority },	/* 100 = getpriority */
	{ 0, (sy_call_t *)nosys },			/* 101 = obsolete osend */
	{ 0, (sy_call_t *)nosys },			/* 102 = obsolete orecv */
	{ 0, (sy_call_t *)nosys },			/* 103 = obsolete osigreturn */
	{ SYF_MPSAFE | AS(bind_args), (sy_call_t *)bind },	/* 104 = bind */
	{ SYF_MPSAFE | AS(setsockopt_args), (sy_call_t *)setsockopt },	/* 105 = setsockopt */
	{ SYF_MPSAFE | AS(listen_args), (sy_call_t *)listen },	/* 106 = listen */
	{ 0, (sy_call_t *)nosys },			/* 107 = obsolete vtimes */
	{ 0, (sy_call_t *)nosys },			/* 108 = obsolete osigvec */
	{ 0, (sy_call_t *)nosys },			/* 109 = obsolete osigblock */
	{ 0, (sy_call_t *)nosys },			/* 110 = obsolete osigsetmask */
	{ 0, (sy_call_t *)nosys },			/* 111 = obsolete osigsuspend */
	{ 0, (sy_call_t *)nosys },			/* 112 = obsolete osigstack */
	{ 0, (sy_call_t *)nosys },			/* 113 = obsolete orecvmsg */
	{ 0, (sy_call_t *)nosys },			/* 114 = obsolete osendmsg */
	{ 0, (sy_call_t *)nosys },			/* 115 = obsolete vtrace */
	{ AS(freebsd32_gettimeofday_args), (sy_call_t *)freebsd32_gettimeofday },	/* 116 = freebsd32_gettimeofday */
	{ AS(freebsd32_getrusage_args), (sy_call_t *)freebsd32_getrusage },	/* 117 = freebsd32_getrusage */
	{ SYF_MPSAFE | AS(getsockopt_args), (sy_call_t *)getsockopt },	/* 118 = getsockopt */
	{ 0, (sy_call_t *)nosys },			/* 119 = resuba */
	{ AS(freebsd32_readv_args), (sy_call_t *)freebsd32_readv },	/* 120 = freebsd32_readv */
	{ AS(freebsd32_writev_args), (sy_call_t *)freebsd32_writev },	/* 121 = freebsd32_writev */
	{ AS(freebsd32_settimeofday_args), (sy_call_t *)freebsd32_settimeofday },	/* 122 = freebsd32_settimeofday */
	{ AS(fchown_args), (sy_call_t *)fchown },	/* 123 = fchown */
	{ AS(fchmod_args), (sy_call_t *)fchmod },	/* 124 = fchmod */
	{ 0, (sy_call_t *)nosys },			/* 125 = obsolete orecvfrom */
	{ SYF_MPSAFE | AS(setreuid_args), (sy_call_t *)setreuid },	/* 126 = setreuid */
	{ SYF_MPSAFE | AS(setregid_args), (sy_call_t *)setregid },	/* 127 = setregid */
	{ AS(rename_args), (sy_call_t *)rename },	/* 128 = rename */
	{ 0, (sy_call_t *)nosys },			/* 129 = obsolete otruncate */
	{ 0, (sy_call_t *)nosys },			/* 130 = obsolete ftruncate */
	{ SYF_MPSAFE | AS(flock_args), (sy_call_t *)flock },	/* 131 = flock */
	{ AS(mkfifo_args), (sy_call_t *)mkfifo },	/* 132 = mkfifo */
	{ SYF_MPSAFE | AS(sendto_args), (sy_call_t *)sendto },	/* 133 = sendto */
	{ SYF_MPSAFE | AS(shutdown_args), (sy_call_t *)shutdown },	/* 134 = shutdown */
	{ SYF_MPSAFE | AS(socketpair_args), (sy_call_t *)socketpair },	/* 135 = socketpair */
	{ AS(mkdir_args), (sy_call_t *)mkdir },		/* 136 = mkdir */
	{ AS(rmdir_args), (sy_call_t *)rmdir },		/* 137 = rmdir */
	{ AS(freebsd32_utimes_args), (sy_call_t *)freebsd32_utimes },	/* 138 = freebsd32_utimes */
	{ 0, (sy_call_t *)nosys },			/* 139 = obsolete 4.2 sigreturn */
	{ AS(freebsd32_adjtime_args), (sy_call_t *)freebsd32_adjtime },	/* 140 = freebsd32_adjtime */
	{ 0, (sy_call_t *)nosys },			/* 141 = obsolete ogetpeername */
	{ 0, (sy_call_t *)nosys },			/* 142 = obsolete ogethostid */
	{ 0, (sy_call_t *)nosys },			/* 143 = obsolete sethostid */
	{ 0, (sy_call_t *)nosys },			/* 144 = obsolete getrlimit */
	{ 0, (sy_call_t *)nosys },			/* 145 = obsolete setrlimit */
	{ 0, (sy_call_t *)nosys },			/* 146 = obsolete killpg */
	{ SYF_MPSAFE | 0, (sy_call_t *)setsid },	/* 147 = setsid */
	{ AS(quotactl_args), (sy_call_t *)quotactl },	/* 148 = quotactl */
	{ 0, (sy_call_t *)nosys },			/* 149 = obsolete oquota */
	{ 0, (sy_call_t *)nosys },			/* 150 = obsolete ogetsockname */
	{ 0, (sy_call_t *)nosys },			/* 151 = sem_lock */
	{ 0, (sy_call_t *)nosys },			/* 152 = sem_wakeup */
	{ 0, (sy_call_t *)nosys },			/* 153 = asyncdaemon */
	{ 0, (sy_call_t *)nosys },			/* 154 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 155 = nfssvc */
	{ 0, (sy_call_t *)nosys },			/* 156 = obsolete ogetdirentries */
	{ AS(freebsd32_statfs_args), (sy_call_t *)freebsd32_statfs },	/* 157 = freebsd32_statfs */
	{ AS(freebsd32_fstatfs_args), (sy_call_t *)freebsd32_fstatfs },	/* 158 = freebsd32_fstatfs */
	{ 0, (sy_call_t *)nosys },			/* 159 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 160 = nosys */
	{ AS(getfh_args), (sy_call_t *)getfh },		/* 161 = getfh */
	{ SYF_MPSAFE | AS(getdomainname_args), (sy_call_t *)getdomainname },	/* 162 = getdomainname */
	{ SYF_MPSAFE | AS(setdomainname_args), (sy_call_t *)setdomainname },	/* 163 = setdomainname */
	{ SYF_MPSAFE | AS(uname_args), (sy_call_t *)uname },	/* 164 = uname */
	{ SYF_MPSAFE | AS(sysarch_args), (sy_call_t *)sysarch },	/* 165 = sysarch */
	{ SYF_MPSAFE | AS(rtprio_args), (sy_call_t *)rtprio },	/* 166 = rtprio */
	{ 0, (sy_call_t *)nosys },			/* 167 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 168 = nosys */
	{ AS(freebsd32_semsys_args), (sy_call_t *)freebsd32_semsys },	/* 169 = freebsd32_semsys */
	{ AS(freebsd32_msgsys_args), (sy_call_t *)freebsd32_msgsys },	/* 170 = freebsd32_msgsys */
	{ AS(freebsd32_shmsys_args), (sy_call_t *)freebsd32_shmsys },	/* 171 = freebsd32_shmsys */
	{ 0, (sy_call_t *)nosys },			/* 172 = nosys */
	{ AS(freebsd32_pread_args), (sy_call_t *)freebsd32_pread },	/* 173 = freebsd32_pread */
	{ AS(freebsd32_pwrite_args), (sy_call_t *)freebsd32_pwrite },	/* 174 = freebsd32_pwrite */
	{ 0, (sy_call_t *)nosys },			/* 175 = nosys */
	{ SYF_MPSAFE | AS(ntp_adjtime_args), (sy_call_t *)ntp_adjtime },	/* 176 = ntp_adjtime */
	{ 0, (sy_call_t *)nosys },			/* 177 = sfork */
	{ 0, (sy_call_t *)nosys },			/* 178 = getdescriptor */
	{ 0, (sy_call_t *)nosys },			/* 179 = setdescriptor */
	{ 0, (sy_call_t *)nosys },			/* 180 = nosys */
	{ SYF_MPSAFE | AS(setgid_args), (sy_call_t *)setgid },	/* 181 = setgid */
	{ SYF_MPSAFE | AS(setegid_args), (sy_call_t *)setegid },	/* 182 = setegid */
	{ SYF_MPSAFE | AS(seteuid_args), (sy_call_t *)seteuid },	/* 183 = seteuid */
	{ 0, (sy_call_t *)nosys },			/* 184 = lfs_bmapv */
	{ 0, (sy_call_t *)nosys },			/* 185 = lfs_markv */
	{ 0, (sy_call_t *)nosys },			/* 186 = lfs_segclean */
	{ 0, (sy_call_t *)nosys },			/* 187 = lfs_segwait */
	{ AS(freebsd32_stat_args), (sy_call_t *)freebsd32_stat },	/* 188 = freebsd32_stat */
	{ AS(freebsd32_fstat_args), (sy_call_t *)freebsd32_fstat },	/* 189 = freebsd32_fstat */
	{ AS(freebsd32_lstat_args), (sy_call_t *)freebsd32_lstat },	/* 190 = freebsd32_lstat */
	{ AS(pathconf_args), (sy_call_t *)pathconf },	/* 191 = pathconf */
	{ SYF_MPSAFE | AS(fpathconf_args), (sy_call_t *)fpathconf },	/* 192 = fpathconf */
	{ 0, (sy_call_t *)nosys },			/* 193 = nosys */
	{ SYF_MPSAFE | AS(__getrlimit_args), (sy_call_t *)getrlimit },	/* 194 = getrlimit */
	{ SYF_MPSAFE | AS(__setrlimit_args), (sy_call_t *)setrlimit },	/* 195 = setrlimit */
	{ AS(getdirentries_args), (sy_call_t *)getdirentries },	/* 196 = getdirentries */
	{ AS(freebsd32_mmap_args), (sy_call_t *)freebsd32_mmap },	/* 197 = freebsd32_mmap */
	{ 0, (sy_call_t *)nosys },			/* 198 = __syscall */
	{ AS(freebsd32_lseek_args), (sy_call_t *)freebsd32_lseek },	/* 199 = freebsd32_lseek */
	{ AS(freebsd32_truncate_args), (sy_call_t *)freebsd32_truncate },	/* 200 = freebsd32_truncate */
	{ AS(freebsd32_ftruncate_args), (sy_call_t *)freebsd32_ftruncate },	/* 201 = freebsd32_ftruncate */
	{ SYF_MPSAFE | AS(freebsd32_sysctl_args), (sy_call_t *)freebsd32_sysctl },	/* 202 = freebsd32_sysctl */
	{ SYF_MPSAFE | AS(mlock_args), (sy_call_t *)mlock },	/* 203 = mlock */
	{ SYF_MPSAFE | AS(munlock_args), (sy_call_t *)munlock },	/* 204 = munlock */
	{ AS(undelete_args), (sy_call_t *)undelete },	/* 205 = undelete */
	{ AS(futimes_args), (sy_call_t *)futimes },	/* 206 = futimes */
	{ SYF_MPSAFE | AS(getpgid_args), (sy_call_t *)getpgid },	/* 207 = getpgid */
	{ 0, (sy_call_t *)nosys },			/* 208 = newreboot */
	{ SYF_MPSAFE | AS(poll_args), (sy_call_t *)poll },	/* 209 = poll */
	{ 0, (sy_call_t *)nosys },			/* 210 =  */
	{ 0, (sy_call_t *)nosys },			/* 211 =  */
	{ 0, (sy_call_t *)nosys },			/* 212 =  */
	{ 0, (sy_call_t *)nosys },			/* 213 =  */
	{ 0, (sy_call_t *)nosys },			/* 214 =  */
	{ 0, (sy_call_t *)nosys },			/* 215 =  */
	{ 0, (sy_call_t *)nosys },			/* 216 =  */
	{ 0, (sy_call_t *)nosys },			/* 217 =  */
	{ 0, (sy_call_t *)nosys },			/* 218 =  */
	{ 0, (sy_call_t *)nosys },			/* 219 =  */
	{ SYF_MPSAFE | AS(__semctl_args), (sy_call_t *)__semctl },	/* 220 = __semctl */
	{ SYF_MPSAFE | AS(semget_args), (sy_call_t *)semget },	/* 221 = semget */
	{ SYF_MPSAFE | AS(semop_args), (sy_call_t *)semop },	/* 222 = semop */
	{ 0, (sy_call_t *)nosys },			/* 223 = semconfig */
	{ SYF_MPSAFE | AS(msgctl_args), (sy_call_t *)msgctl },	/* 224 = msgctl */
	{ SYF_MPSAFE | AS(msgget_args), (sy_call_t *)msgget },	/* 225 = msgget */
	{ SYF_MPSAFE | AS(msgsnd_args), (sy_call_t *)msgsnd },	/* 226 = msgsnd */
	{ SYF_MPSAFE | AS(msgrcv_args), (sy_call_t *)msgrcv },	/* 227 = msgrcv */
	{ SYF_MPSAFE | AS(shmat_args), (sy_call_t *)shmat },	/* 228 = shmat */
	{ SYF_MPSAFE | AS(shmctl_args), (sy_call_t *)shmctl },	/* 229 = shmctl */
	{ SYF_MPSAFE | AS(shmdt_args), (sy_call_t *)shmdt },	/* 230 = shmdt */
	{ SYF_MPSAFE | AS(shmget_args), (sy_call_t *)shmget },	/* 231 = shmget */
	{ SYF_MPSAFE | AS(clock_gettime_args), (sy_call_t *)clock_gettime },	/* 232 = clock_gettime */
	{ SYF_MPSAFE | AS(clock_settime_args), (sy_call_t *)clock_settime },	/* 233 = clock_settime */
	{ SYF_MPSAFE | AS(clock_getres_args), (sy_call_t *)clock_getres },	/* 234 = clock_getres */
	{ 0, (sy_call_t *)nosys },			/* 235 = timer_create */
	{ 0, (sy_call_t *)nosys },			/* 236 = timer_delete */
	{ 0, (sy_call_t *)nosys },			/* 237 = timer_settime */
	{ 0, (sy_call_t *)nosys },			/* 238 = timer_gettime */
	{ 0, (sy_call_t *)nosys },			/* 239 = timer_getoverrun */
	{ SYF_MPSAFE | AS(nanosleep_args), (sy_call_t *)nanosleep },	/* 240 = nanosleep */
	{ 0, (sy_call_t *)nosys },			/* 241 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 242 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 243 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 244 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 245 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 246 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 247 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 248 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 249 = nosys */
	{ SYF_MPSAFE | AS(minherit_args), (sy_call_t *)minherit },	/* 250 = minherit */
	{ SYF_MPSAFE | AS(rfork_args), (sy_call_t *)rfork },	/* 251 = rfork */
	{ SYF_MPSAFE | AS(openbsd_poll_args), (sy_call_t *)openbsd_poll },	/* 252 = openbsd_poll */
	{ SYF_MPSAFE | 0, (sy_call_t *)issetugid },	/* 253 = issetugid */
	{ AS(lchown_args), (sy_call_t *)lchown },	/* 254 = lchown */
	{ 0, (sy_call_t *)nosys },			/* 255 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 256 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 257 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 258 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 259 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 260 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 261 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 262 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 263 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 264 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 265 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 266 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 267 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 268 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 269 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 270 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 271 = nosys */
	{ AS(getdents_args), (sy_call_t *)getdents },	/* 272 = getdents */
	{ 0, (sy_call_t *)nosys },			/* 273 = nosys */
	{ AS(lchmod_args), (sy_call_t *)lchmod },	/* 274 = lchmod */
	{ AS(lchown_args), (sy_call_t *)lchown },	/* 275 = netbsd_lchown */
	{ AS(lutimes_args), (sy_call_t *)lutimes },	/* 276 = lutimes */
	{ SYF_MPSAFE | AS(msync_args), (sy_call_t *)msync },	/* 277 = netbsd_msync */
	{ AS(nstat_args), (sy_call_t *)nstat },		/* 278 = nstat */
	{ SYF_MPSAFE | AS(nfstat_args), (sy_call_t *)nfstat },	/* 279 = nfstat */
	{ AS(nlstat_args), (sy_call_t *)nlstat },	/* 280 = nlstat */
	{ 0, (sy_call_t *)nosys },			/* 281 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 282 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 283 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 284 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 285 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 286 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 287 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 288 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 289 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 290 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 291 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 292 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 293 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 294 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 295 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 296 = nosys */
	{ AS(fhstatfs_args), (sy_call_t *)fhstatfs },	/* 297 = fhstatfs */
	{ AS(fhopen_args), (sy_call_t *)fhopen },	/* 298 = fhopen */
	{ AS(fhstat_args), (sy_call_t *)fhstat },	/* 299 = fhstat */
	{ SYF_MPSAFE | AS(modnext_args), (sy_call_t *)modnext },	/* 300 = modnext */
	{ SYF_MPSAFE | AS(modstat_args), (sy_call_t *)modstat },	/* 301 = modstat */
	{ SYF_MPSAFE | AS(modfnext_args), (sy_call_t *)modfnext },	/* 302 = modfnext */
	{ SYF_MPSAFE | AS(modfind_args), (sy_call_t *)modfind },	/* 303 = modfind */
	{ SYF_MPSAFE | AS(kldload_args), (sy_call_t *)kldload },	/* 304 = kldload */
	{ SYF_MPSAFE | AS(kldunload_args), (sy_call_t *)kldunload },	/* 305 = kldunload */
	{ SYF_MPSAFE | AS(kldfind_args), (sy_call_t *)kldfind },	/* 306 = kldfind */
	{ SYF_MPSAFE | AS(kldnext_args), (sy_call_t *)kldnext },	/* 307 = kldnext */
	{ SYF_MPSAFE | AS(kldstat_args), (sy_call_t *)kldstat },	/* 308 = kldstat */
	{ SYF_MPSAFE | AS(kldfirstmod_args), (sy_call_t *)kldfirstmod },	/* 309 = kldfirstmod */
	{ SYF_MPSAFE | AS(getsid_args), (sy_call_t *)getsid },	/* 310 = getsid */
	{ SYF_MPSAFE | AS(setresuid_args), (sy_call_t *)setresuid },	/* 311 = setresuid */
	{ SYF_MPSAFE | AS(setresgid_args), (sy_call_t *)setresgid },	/* 312 = setresgid */
	{ 0, (sy_call_t *)nosys },			/* 313 = obsolete signanosleep */
	{ 0, (sy_call_t *)nosys },			/* 314 = aio_return */
	{ 0, (sy_call_t *)nosys },			/* 315 = aio_suspend */
	{ 0, (sy_call_t *)nosys },			/* 316 = aio_cancel */
	{ 0, (sy_call_t *)nosys },			/* 317 = aio_error */
	{ 0, (sy_call_t *)nosys },			/* 318 = aio_read */
	{ 0, (sy_call_t *)nosys },			/* 319 = aio_write */
	{ 0, (sy_call_t *)nosys },			/* 320 = lio_listio */
	{ SYF_MPSAFE | 0, (sy_call_t *)yield },		/* 321 = yield */
	{ 0, (sy_call_t *)nosys },			/* 322 = obsolete thr_sleep */
	{ 0, (sy_call_t *)nosys },			/* 323 = obsolete thr_wakeup */
	{ SYF_MPSAFE | AS(mlockall_args), (sy_call_t *)mlockall },	/* 324 = mlockall */
	{ SYF_MPSAFE | 0, (sy_call_t *)munlockall },	/* 325 = munlockall */
	{ AS(__getcwd_args), (sy_call_t *)__getcwd },	/* 326 = __getcwd */
	{ SYF_MPSAFE | AS(sched_setparam_args), (sy_call_t *)sched_setparam },	/* 327 = sched_setparam */
	{ SYF_MPSAFE | AS(sched_getparam_args), (sy_call_t *)sched_getparam },	/* 328 = sched_getparam */
	{ SYF_MPSAFE | AS(sched_setscheduler_args), (sy_call_t *)sched_setscheduler },	/* 329 = sched_setscheduler */
	{ SYF_MPSAFE | AS(sched_getscheduler_args), (sy_call_t *)sched_getscheduler },	/* 330 = sched_getscheduler */
	{ SYF_MPSAFE | 0, (sy_call_t *)sched_yield },	/* 331 = sched_yield */
	{ SYF_MPSAFE | AS(sched_get_priority_max_args), (sy_call_t *)sched_get_priority_max },	/* 332 = sched_get_priority_max */
	{ SYF_MPSAFE | AS(sched_get_priority_min_args), (sy_call_t *)sched_get_priority_min },	/* 333 = sched_get_priority_min */
	{ SYF_MPSAFE | AS(sched_rr_get_interval_args), (sy_call_t *)sched_rr_get_interval },	/* 334 = sched_rr_get_interval */
	{ SYF_MPSAFE | AS(utrace_args), (sy_call_t *)utrace },	/* 335 = utrace */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sendfile_args),freebsd32_sendfile) },	/* 336 = old freebsd32_sendfile */
	{ AS(kldsym_args), (sy_call_t *)kldsym },	/* 337 = kldsym */
	{ SYF_MPSAFE | AS(jail_args), (sy_call_t *)jail },	/* 338 = jail */
	{ 0, (sy_call_t *)nosys },			/* 339 = pioctl */
	{ SYF_MPSAFE | AS(sigprocmask_args), (sy_call_t *)sigprocmask },	/* 340 = sigprocmask */
	{ SYF_MPSAFE | AS(sigsuspend_args), (sy_call_t *)sigsuspend },	/* 341 = sigsuspend */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sigaction_args),freebsd32_sigaction) },	/* 342 = old freebsd32_sigaction */
	{ SYF_MPSAFE | AS(sigpending_args), (sy_call_t *)sigpending },	/* 343 = sigpending */
	{ compat4(SYF_MPSAFE | AS(freebsd4_freebsd32_sigreturn_args),freebsd32_sigreturn) },	/* 344 = old freebsd32_sigreturn */
	{ 0, (sy_call_t *)nosys },			/* 345 = sigtimedwait */
	{ 0, (sy_call_t *)nosys },			/* 346 = sigwaitinfo */
	{ SYF_MPSAFE | AS(__acl_get_file_args), (sy_call_t *)__acl_get_file },	/* 347 = __acl_get_file */
	{ SYF_MPSAFE | AS(__acl_set_file_args), (sy_call_t *)__acl_set_file },	/* 348 = __acl_set_file */
	{ SYF_MPSAFE | AS(__acl_get_fd_args), (sy_call_t *)__acl_get_fd },	/* 349 = __acl_get_fd */
	{ SYF_MPSAFE | AS(__acl_set_fd_args), (sy_call_t *)__acl_set_fd },	/* 350 = __acl_set_fd */
	{ SYF_MPSAFE | AS(__acl_delete_file_args), (sy_call_t *)__acl_delete_file },	/* 351 = __acl_delete_file */
	{ SYF_MPSAFE | AS(__acl_delete_fd_args), (sy_call_t *)__acl_delete_fd },	/* 352 = __acl_delete_fd */
	{ SYF_MPSAFE | AS(__acl_aclcheck_file_args), (sy_call_t *)__acl_aclcheck_file },	/* 353 = __acl_aclcheck_file */
	{ SYF_MPSAFE | AS(__acl_aclcheck_fd_args), (sy_call_t *)__acl_aclcheck_fd },	/* 354 = __acl_aclcheck_fd */
	{ AS(extattrctl_args), (sy_call_t *)extattrctl },	/* 355 = extattrctl */
	{ AS(extattr_set_file_args), (sy_call_t *)extattr_set_file },	/* 356 = extattr_set_file */
	{ AS(extattr_get_file_args), (sy_call_t *)extattr_get_file },	/* 357 = extattr_get_file */
	{ AS(extattr_delete_file_args), (sy_call_t *)extattr_delete_file },	/* 358 = extattr_delete_file */
	{ 0, (sy_call_t *)nosys },			/* 359 = aio_waitcomplete */
	{ SYF_MPSAFE | AS(getresuid_args), (sy_call_t *)getresuid },	/* 360 = getresuid */
	{ SYF_MPSAFE | AS(getresgid_args), (sy_call_t *)getresgid },	/* 361 = getresgid */
	{ SYF_MPSAFE | 0, (sy_call_t *)kqueue },	/* 362 = kqueue */
	{ SYF_MPSAFE | AS(freebsd32_kevent_args), (sy_call_t *)freebsd32_kevent },	/* 363 = freebsd32_kevent */
	{ 0, (sy_call_t *)nosys },			/* 364 = __cap_get_proc */
	{ 0, (sy_call_t *)nosys },			/* 365 = __cap_set_proc */
	{ 0, (sy_call_t *)nosys },			/* 366 = __cap_get_fd */
	{ 0, (sy_call_t *)nosys },			/* 367 = __cap_get_file */
	{ 0, (sy_call_t *)nosys },			/* 368 = __cap_set_fd */
	{ 0, (sy_call_t *)nosys },			/* 369 = __cap_set_file */
	{ 0, (sy_call_t *)nosys },			/* 370 = lkmressys */
	{ AS(extattr_set_fd_args), (sy_call_t *)extattr_set_fd },	/* 371 = extattr_set_fd */
	{ AS(extattr_get_fd_args), (sy_call_t *)extattr_get_fd },	/* 372 = extattr_get_fd */
	{ AS(extattr_delete_fd_args), (sy_call_t *)extattr_delete_fd },	/* 373 = extattr_delete_fd */
	{ SYF_MPSAFE | AS(__setugid_args), (sy_call_t *)__setugid },	/* 374 = __setugid */
	{ 0, (sy_call_t *)nosys },			/* 375 = nfsclnt */
	{ AS(eaccess_args), (sy_call_t *)eaccess },	/* 376 = eaccess */
	{ 0, (sy_call_t *)nosys },			/* 377 = afs_syscall */
	{ AS(nmount_args), (sy_call_t *)nmount },	/* 378 = nmount */
	{ 0, (sy_call_t *)kse_exit },			/* 379 = kse_exit */
	{ AS(kse_wakeup_args), (sy_call_t *)kse_wakeup },	/* 380 = kse_wakeup */
	{ AS(kse_create_args), (sy_call_t *)kse_create },	/* 381 = kse_create */
	{ AS(kse_thr_interrupt_args), (sy_call_t *)kse_thr_interrupt },	/* 382 = kse_thr_interrupt */
	{ 0, (sy_call_t *)kse_release },		/* 383 = kse_release */
	{ 0, (sy_call_t *)nosys },			/* 384 = __mac_get_proc */
	{ 0, (sy_call_t *)nosys },			/* 385 = __mac_set_proc */
	{ 0, (sy_call_t *)nosys },			/* 386 = __mac_get_fd */
	{ 0, (sy_call_t *)nosys },			/* 387 = __mac_get_file */
	{ 0, (sy_call_t *)nosys },			/* 388 = __mac_set_fd */
	{ 0, (sy_call_t *)nosys },			/* 389 = __mac_set_file */
	{ AS(kenv_args), (sy_call_t *)kenv },		/* 390 = kenv */
	{ AS(lchflags_args), (sy_call_t *)lchflags },	/* 391 = lchflags */
	{ AS(uuidgen_args), (sy_call_t *)uuidgen },	/* 392 = uuidgen */
	{ SYF_MPSAFE | AS(freebsd32_sendfile_args), (sy_call_t *)freebsd32_sendfile },	/* 393 = freebsd32_sendfile */
	{ 0, (sy_call_t *)nosys },			/* 394 = mac_syscall */
	{ 0, (sy_call_t *)nosys },			/* 395 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 396 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 397 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 398 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 399 = nosys */
	{ 0, (sy_call_t *)nosys },			/* 400 = ksem_close */
	{ 0, (sy_call_t *)nosys },			/* 401 = ksem_post */
	{ 0, (sy_call_t *)nosys },			/* 402 = ksem_wait */
	{ 0, (sy_call_t *)nosys },			/* 403 = ksem_trywait */
	{ 0, (sy_call_t *)nosys },			/* 404 = ksem_init */
	{ 0, (sy_call_t *)nosys },			/* 405 = ksem_open */
	{ 0, (sy_call_t *)nosys },			/* 406 = ksem_unlink */
	{ 0, (sy_call_t *)nosys },			/* 407 = ksem_getvalue */
	{ 0, (sy_call_t *)nosys },			/* 408 = ksem_destroy */
	{ 0, (sy_call_t *)nosys },			/* 409 = __mac_get_pid */
	{ 0, (sy_call_t *)nosys },			/* 410 = __mac_get_link */
	{ 0, (sy_call_t *)nosys },			/* 411 = __mac_set_link */
	{ 0, (sy_call_t *)nosys },			/* 412 = extattr_set_link */
	{ 0, (sy_call_t *)nosys },			/* 413 = extattr_get_link */
	{ 0, (sy_call_t *)nosys },			/* 414 = extattr_delete_link */
	{ 0, (sy_call_t *)nosys },			/* 415 = __mac_execve */
	{ AS(freebsd32_sigaction_args), (sy_call_t *)freebsd32_sigaction },	/* 416 = freebsd32_sigaction */
	{ SYF_MPSAFE | AS(freebsd32_sigreturn_args), (sy_call_t *)freebsd32_sigreturn },	/* 417 = freebsd32_sigreturn */
	{ 0, (sy_call_t *)nosys },			/* 418 = __xstat */
	{ 0, (sy_call_t *)nosys },			/* 419 = __xfstat */
	{ 0, (sy_call_t *)nosys },			/* 420 = __xlstat */
	{ 0, (sy_call_t *)nosys },			/* 421 = getcontext */
	{ 0, (sy_call_t *)nosys },			/* 422 = setcontext */
	{ 0, (sy_call_t *)nosys },			/* 423 = swapcontext */
	{ 0, (sy_call_t *)nosys },			/* 424 = swapoff */
	{ 0, (sy_call_t *)nosys },			/* 425 = __acl_get_link */
	{ 0, (sy_call_t *)nosys },			/* 426 = __acl_set_link */
	{ 0, (sy_call_t *)nosys },			/* 427 = __acl_delete_link */
	{ 0, (sy_call_t *)nosys },			/* 428 = __acl_aclcheck_link */
	{ 0, (sy_call_t *)nosys },			/* 429 = sigwait */
	{ SYF_MPSAFE | AS(thr_create_args), (sy_call_t *)thr_create },	/* 430 = thr_create */
	{ SYF_MPSAFE | 0, (sy_call_t *)thr_exit },	/* 431 = thr_exit */
	{ SYF_MPSAFE | AS(thr_self_args), (sy_call_t *)thr_self },	/* 432 = thr_self */
	{ SYF_MPSAFE | AS(thr_kill_args), (sy_call_t *)thr_kill },	/* 433 = thr_kill */
	{ SYF_MPSAFE | AS(_umtx_lock_args), (sy_call_t *)_umtx_lock },	/* 434 = _umtx_lock */
	{ SYF_MPSAFE | AS(_umtx_unlock_args), (sy_call_t *)_umtx_unlock },	/* 435 = _umtx_unlock */
	{ SYF_MPSAFE | AS(jail_attach_args), (sy_call_t *)jail_attach },	/* 436 = jail_attach */
	{ 0, (sy_call_t *)nosys },			/* 437 = extattr_list_fd */
	{ 0, (sy_call_t *)nosys },			/* 438 = extattr_list_file */
	{ 0, (sy_call_t *)nosys },			/* 439 = extattr_list_link */
	{ 0, (sy_call_t *)nosys },			/* 440 = kse_switchin */
};
