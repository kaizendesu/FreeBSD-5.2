/*-
 * Copyright (c) 2000,2001,2002 S�ren Schmidt <sos@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer,
 *    without modification, immediately at the beginning of the file.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/types.h>
#include <sys/ata.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

const char *mode2str(int mode);
int str2mode(char *str);
void usage(void);
int version(int ver);
void param_print(struct ata_params *parm);
void cap_print(struct ata_params *parm);
int ata_cap_print(int fd, int channel, int device);
int info_print(int fd, int channel, int prchan);

const char *
mode2str(int mode)
{
	switch (mode) {
	case ATA_PIO: return "BIOSPIO";
	case ATA_PIO0: return "PIO0";
	case ATA_PIO1: return "PIO1";
	case ATA_PIO2: return "PIO2";
	case ATA_PIO3: return "PIO3";
	case ATA_PIO4: return "PIO4";
	case ATA_WDMA2: return "WDMA2";
	case ATA_UDMA2: return "UDMA33";
 	case ATA_UDMA4: return "UDMA66";
	case ATA_UDMA5: return "UDMA100";
	case ATA_UDMA6: return "UDMA133";
	case ATA_SA150: return "SATA150";
	case ATA_DMA: return "BIOSDMA";
	default: return "???";
	}
}

int
str2mode(char *str)
{
	if (!strcasecmp(str, "BIOSPIO")) return ATA_PIO;
	if (!strcasecmp(str, "PIO0")) return ATA_PIO0;
	if (!strcasecmp(str, "PIO1")) return ATA_PIO1;
	if (!strcasecmp(str, "PIO2")) return ATA_PIO2;
	if (!strcasecmp(str, "PIO3")) return ATA_PIO3;
	if (!strcasecmp(str, "PIO4")) return ATA_PIO4;
	if (!strcasecmp(str, "WDMA2")) return ATA_WDMA2;
	if (!strcasecmp(str, "UDMA2")) return ATA_UDMA2;
	if (!strcasecmp(str, "UDMA33")) return ATA_UDMA2;
	if (!strcasecmp(str, "UDMA4")) return ATA_UDMA4;
	if (!strcasecmp(str, "UDMA66")) return ATA_UDMA4;
	if (!strcasecmp(str, "UDMA5")) return ATA_UDMA5;
	if (!strcasecmp(str, "UDMA100")) return ATA_UDMA5;
	if (!strcasecmp(str, "UDMA6")) return ATA_UDMA6;
	if (!strcasecmp(str, "UDMA133")) return ATA_UDMA6;
	if (!strcasecmp(str, "BIOSDMA")) return ATA_DMA;
	return -1;
}

void
usage()
{
	fprintf(stderr, "usage: atacontrol <command> channel [args]\n");
	exit(EX_USAGE);
}

int
version(int ver)
{
	int bit;
    
	if (ver == 0xffff)
		return 0;
	for (bit = 15; bit >= 0; bit--)
		if (ver & (1<<bit))
			return bit;
    	return 0;
}

void
param_print(struct ata_params *parm)
{
	printf("<%.40s/%.8s> ATA/ATAPI rev %d\n",
		parm->model, parm->revision, version(parm->version_major)); 
}

void
cap_print(struct ata_params *parm)
{
	u_int32_t lbasize = (u_int32_t)parm->lba_size_1 |
				((u_int32_t)parm->lba_size_2 << 16);

	u_int64_t lbasize48 = ((u_int64_t)parm->lba_size48_1) |
				((u_int64_t)parm->lba_size48_2 << 16) |
				((u_int64_t)parm->lba_size48_3 << 32) |
				((u_int64_t)parm->lba_size48_4 << 48);
 
	printf("\n");
	printf("ATA/ATAPI revision    %d\n", version(parm->version_major));
	printf("device model          %.40s\n", parm->model);
	printf("serial number         %.20s\n", parm->serial);
	printf("firmware revision     %.8s\n", parm->revision);

	printf("cylinders             %d\n", parm->cylinders);
	printf("heads                 %d\n", parm->heads);
	printf("sectors/track         %d\n", parm->sectors);	
	
	printf("lba%ssupported         ",
		parm->capabilities1 & ATA_SUPPORT_LBA ? " " : " not ");
	if (lbasize)
		printf("%d sectors\n", lbasize);
	else
		printf("\n");

	printf("lba48%ssupported         ",
		parm->support.command2 & ATA_SUPPORT_ADDRESS48 ? " " : " not ");
	if (lbasize48)
		printf("%ju sectors\n", (uintmax_t)lbasize48);	
	else
		printf("\n");

	printf("dma%ssupported\n",
		parm->capabilities1 & ATA_SUPPORT_DMA ? " " : " not");

	printf("overlap%ssupported\n",
		parm->capabilities1 & ATA_SUPPORT_OVERLAP ? " " : " not ");
  
	printf("\nFeature                      "
		"Support  Enable    Value   Vendor\n");

	printf("write cache                    %s	%s\n",
		parm->support.command1 & ATA_SUPPORT_WRITECACHE ? "yes" : "no",
		parm->enabled.command1 & ATA_SUPPORT_WRITECACHE ? "yes" : "no");

	printf("read ahead                     %s	%s\n",
		parm->support.command1 & ATA_SUPPORT_LOOKAHEAD ? "yes" : "no",
		parm->enabled.command1 & ATA_SUPPORT_LOOKAHEAD ? "yes" : "no");	

	printf("dma queued                     %s	%s	%d/0x%02X\n",
		parm->support.command2 & ATA_SUPPORT_QUEUED ? "yes" : "no",
		parm->enabled.command2 & ATA_SUPPORT_QUEUED ? "yes" : "no",
		ATA_QUEUE_LEN(parm->queue), ATA_QUEUE_LEN(parm->queue));	

	printf("SMART                          %s	%s\n",
		parm->support.command1 & ATA_SUPPORT_SMART ? "yes" : "no",
		parm->enabled.command1 & ATA_SUPPORT_SMART ? "yes" : "no");

	printf("microcode download             %s	%s\n",
		parm->support.command2 & ATA_SUPPORT_MICROCODE ? "yes" : "no",
		parm->enabled.command2 & ATA_SUPPORT_MICROCODE ? "yes" : "no");	

	printf("security                       %s	%s\n",
		parm->support.command1 & ATA_SUPPORT_SECURITY ? "yes" : "no",
		parm->enabled.command1 & ATA_SUPPORT_SECURITY ? "yes" : "no");	

	printf("power management               %s	%s\n",
		parm->support.command1 & ATA_SUPPORT_POWERMGT ? "yes" : "no",
		parm->enabled.command1 & ATA_SUPPORT_POWERMGT ? "yes" : "no");	

	printf("advanced power management      %s	%s	%d/0x%02X\n",
		parm->support.command2 & ATA_SUPPORT_APM ? "yes" : "no",
		parm->enabled.command2 & ATA_SUPPORT_APM ? "yes" : "no",
		parm->apm_value, parm->apm_value);

	printf("automatic acoustic management  %s	%s	"
		"%d/0x%02X	%d/0x%02X\n",
		parm->support.command2 & ATA_SUPPORT_AUTOACOUSTIC ? "yes" :"no",
		parm->enabled.command2 & ATA_SUPPORT_AUTOACOUSTIC ? "yes" :"no",
		ATA_ACOUSTIC_CURRENT(parm->acoustic),
		ATA_ACOUSTIC_CURRENT(parm->acoustic),
		ATA_ACOUSTIC_VENDOR(parm->acoustic),
		ATA_ACOUSTIC_VENDOR(parm->acoustic));
}

int
ata_cap_print(int fd, int channel, int device)
{
	struct ata_cmd iocmd;

	if (device < 0 || device > 1)
		return ENXIO;

	bzero(&iocmd, sizeof(struct ata_cmd));

	iocmd.channel = channel;
	iocmd.device = device;
	iocmd.cmd = ATAGPARM;

	if (ioctl(fd, IOCATA, &iocmd) < 0)
		return errno;

	printf("ATA channel %d, %s", channel, device==0 ? "Master" : "Slave");

	if (iocmd.u.param.type[device]) {
		printf(", device %s:\n", iocmd.u.param.name[device]);
		cap_print(&iocmd.u.param.params[device]);
	}
	else
		printf(": no device present\n");
	return 0;
}

int
info_print(int fd, int channel, int prchan)
{
	struct ata_cmd iocmd;

	bzero(&iocmd, sizeof(struct ata_cmd));
	iocmd.channel = channel;
	iocmd.device = -1;
	iocmd.cmd = ATAGPARM;
	if (ioctl(fd, IOCATA, &iocmd) < 0)
		return errno;
	if (prchan)
		printf("ATA channel %d:\n", channel);
	printf("%sMaster: ", prchan ? "    " : "");
	if (iocmd.u.param.type[0]) {
		printf("%4.4s ", iocmd.u.param.name[0]);
		param_print(&iocmd.u.param.params[0]);
	}
	else
		printf("     no device present\n");
	printf("%sSlave:  ", prchan ? "    " : "");
	if (iocmd.u.param.type[1]) {
		printf("%4.4s ", iocmd.u.param.name[1]);
		param_print(&iocmd.u.param.params[1]);
	}
	else
		printf("     no device present\n");
	return 0;
}

int
main(int argc, char **argv)
{
	struct ata_cmd iocmd;
	int fd, maxunit, unit;

	if ((fd = open("/dev/ata", O_RDWR)) < 0)
		err(1, "control device not found");

	if (argc < 2)
		usage();

	bzero(&iocmd, sizeof(struct ata_cmd));

	if (argc > 2 && strcmp(argv[1], "create")) {
		int chan;

		if (!strcmp(argv[1], "addspare") ||
		    !strcmp(argv[1], "delete") ||
		    !strcmp(argv[1], "rebuild") ||
		    !strcmp(argv[1], "status")) {
			if (!(sscanf(argv[2], "%d", &chan) == 1 ||
			      sscanf(argv[2], "ar%d", &chan) == 1)) {
				fprintf(stderr, "atacontrol: Invalid RAID device\n");
				exit(EX_USAGE);
			}
		}
		else {
			if (!(sscanf(argv[2], "%d", &chan) == 1 ||
			      sscanf(argv[2], "ata%d", &chan) == 1)) {
				fprintf(stderr, "atacontrol: Invalid ATA channel\n");
				exit(EX_USAGE);
			}
		}
		iocmd.channel = chan;
	}

	if (!strcmp(argv[1], "list") && argc == 2) {
		iocmd.cmd = ATAGMAXCHANNEL;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATAGMAXCHANNEL)");
		maxunit = iocmd.u.maxchan;
		for (unit = 0; unit < maxunit; unit++)
			info_print(fd, unit, 1);
	}
	else if (!strcmp(argv[1], "info") && argc == 3) {
		info_print(fd, iocmd.channel, 0);
	}
	else if (!strcmp(argv[1], "cap") && argc == 4) {
		ata_cap_print(fd, iocmd.channel, atoi(argv[3]));
	}
	else if (!strcmp(argv[1], "enclosure") && argc == 4) {
		iocmd.device = atoi(argv[3]);
		iocmd.cmd = ATAENCSTAT;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATAENCSTAT)");
		printf("fan RPM: %d temp: %.1f 5V: %.2f 12V: %.2f\n",
			iocmd.u.enclosure.fan,
			(double)iocmd.u.enclosure.temp / 10,
			(double)iocmd.u.enclosure.v05 / 1000,
			(double)iocmd.u.enclosure.v12 / 1000);
	}
	else if (!strcmp(argv[1], "detach") && argc == 3) {
		iocmd.cmd = ATADETACH;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATADETACH)");
	}
	else if (!strcmp(argv[1], "attach") && argc == 3) {
		iocmd.cmd = ATAATTACH;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATAATTACH)");
		info_print(fd, iocmd.channel, 0);
	}
	else if (!strcmp(argv[1], "reinit") && argc == 3) {
		iocmd.cmd = ATAREINIT;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			warn("ioctl(ATAREINIT)");
		info_print(fd, iocmd.channel, 0);
	}
	else if (!strcmp(argv[1], "create")) {
		int disk, dev, offset;

		iocmd.cmd = ATARAIDCREATE;
		if (argc > 2) {
			if (!strcmp(argv[2], "RAID0") ||
			    !strcmp(argv[2], "stripe"))
				iocmd.u.raid_setup.type = 1;
			if (!strcmp(argv[2], "RAID1") ||
			    !strcmp(argv[2],"mirror"))
				iocmd.u.raid_setup.type = 2;
			if (!strcmp(argv[2], "RAID0+1"))
				iocmd.u.raid_setup.type = 3;
			if (!strcmp(argv[2], "SPAN") ||
			    !strcmp(argv[2], "JBOD"))
				iocmd.u.raid_setup.type = 4;
		}
		if (!iocmd.u.raid_setup.type) {
			fprintf(stderr, "atacontrol: Invalid RAID type\n");
			fprintf(stderr, "atacontrol: Valid RAID types : \n");
			fprintf(stderr, "            RAID0 | stripe | RAID1 | mirror "
						    "| RAID0+1 | SPAN | JBOD\n");
			exit(EX_USAGE);
		}
		
		if (iocmd.u.raid_setup.type & 1) {
			if (argc < 4 || 
			    !sscanf(argv[3], "%d",
				    &iocmd.u.raid_setup.interleave) == 1) {
				fprintf(stderr, "atacontrol: Invalid interleave\n");
				exit(EX_USAGE);
			}
			offset = 4;
		}
		else
			offset = 3;
		
		for (disk = 0; disk < 16 && (offset + disk) < argc; disk++) {
			if (!(sscanf(argv[offset + disk], "%d", &dev) == 1 ||
			      sscanf(argv[offset + disk], "ad%d", &dev) == 1)) {
				fprintf(stderr,
					"atacontrol: Invalid device %s\n",
					argv[offset + disk]);
				exit(EX_USAGE);
			}
			iocmd.u.raid_setup.disks[disk] = dev;
		}

		if(disk < 2) {
			fprintf(stderr, "atacontrol: At least 2 disks must be "
				"specified to create RAID\n");
			exit(EX_USAGE);
		}

		iocmd.u.raid_setup.total_disks = disk;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATARAIDCREATE)");
		else
			printf("ar%d created\n", iocmd.u.raid_setup.unit);
	}
	else if (!strcmp(argv[1], "delete") && argc == 3) {
		iocmd.cmd = ATARAIDDELETE;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			warn("ioctl(ATARAIDDELETE)");
	}
	else if (!strcmp(argv[1], "addspare") && argc == 4) {
		int dev;

		iocmd.cmd = ATARAIDADDSPARE;
		if (!(sscanf(argv[3], "%d", &dev) == 1 ||
		      sscanf(argv[3], "ad%d", &dev) == 1)) {
			fprintf(stderr,
				"atacontrol: Invalid device %s\n", argv[3]);
			usage();
		}
		iocmd.u.raid_spare.disk = dev;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			warn("ioctl(ATARAIDADDSPARE)");
	}
	else if (!strcmp(argv[1], "rebuild") && argc == 3) {
		iocmd.cmd = ATARAIDREBUILD;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			warn("ioctl(ATARAIDREBUILD)");
	}
	else if (!strcmp(argv[1], "status") && argc == 3) {
		int i;

		iocmd.cmd = ATARAIDSTATUS;
		if (ioctl(fd, IOCATA, &iocmd) < 0)
			err(1, "ioctl(ATARAIDSTATUS)");
		printf("ar%d: ATA ", iocmd.channel);
		switch (iocmd.u.raid_status.type) {
		case AR_RAID0:
			printf("RAID0 stripesize=%d",
				iocmd.u.raid_status.interleave);
			break;
		case AR_RAID1:
			printf("RAID1");
			break;
		case AR_RAID0 | AR_RAID1:
			printf("RAID0+1 stripesize=%d",
				iocmd.u.raid_status.interleave);
			break;
		case AR_SPAN:
			printf("SPAN");
			break;
		}
		printf(" subdisks: ");
		for (i = 0; i < iocmd.u.raid_status.total_disks; i++) {
			if (iocmd.u.raid_status.disks[i] >= 0)
				printf("ad%d ", iocmd.u.raid_status.disks[i]);
			else
				printf("DOWN ");
		}
		printf("status: ");
		switch (iocmd.u.raid_status.status) {
		case AR_READY:
			printf("READY\n");
			break;
		case AR_READY | AR_DEGRADED:
			printf("DEGRADED\n");
			break;
		case AR_READY | AR_DEGRADED | AR_REBUILDING:
			printf("REBUILDING %d%% completed\n",
				iocmd.u.raid_status.progress);
			break;
		default:
			printf("BROKEN\n");
		}
	}
	else if (!strcmp(argv[1], "mode") && (argc == 3 || argc == 5)) {
		if (argc == 5) {
			iocmd.cmd = ATASMODE;
			iocmd.device = -1;
			iocmd.u.mode.mode[0] = str2mode(argv[3]);
			iocmd.u.mode.mode[1] = str2mode(argv[4]);
			if (ioctl(fd, IOCATA, &iocmd) < 0)
				warn("ioctl(ATASMODE)");
		}
		if (argc == 3 || argc == 5) {
			iocmd.cmd = ATAGMODE;
			iocmd.device = -1;
			if (ioctl(fd, IOCATA, &iocmd) < 0)
				err(1, "ioctl(ATAGMODE)");
			printf("Master = %s \nSlave  = %s\n",
				mode2str(iocmd.u.mode.mode[0]), 
				mode2str(iocmd.u.mode.mode[1]));
		}
	}
	else
	    	usage();
	exit(EX_OK);
}
