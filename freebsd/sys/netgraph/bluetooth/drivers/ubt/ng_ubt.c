/*
 * ng_ubt.c
 *
 * Copyright (c) 2001-2002 Maksim Yevmenkin <m_evmenkin@yahoo.com>
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: ng_ubt.c,v 1.16 2003/10/10 19:15:06 max Exp $
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/bus.h>
#include <sys/conf.h>
#include <sys/endian.h>
#include <sys/filio.h>
#include <sys/fcntl.h>
#include <sys/mbuf.h>
#include <sys/malloc.h>
#include <sys/kernel.h>
#include <sys/poll.h>
#include <sys/uio.h>
#include <machine/bus.h>

#include <dev/usb/usb.h>
#include <dev/usb/usbdi.h>
#include <dev/usb/usbdi_util.h>
#include <dev/usb/usbdivar.h>
#include <dev/usb/usbdevs.h>

#include <netgraph/ng_message.h>
#include <netgraph/netgraph.h>
#include <netgraph/ng_parse.h>
#include <ng_bluetooth.h>
#include <ng_hci.h>
#include "ng_ubt.h"
#include "ng_ubt_var.h"

/*
 * USB methods
 */

USB_DECLARE_DRIVER(ubt);

Static int         ubt_modevent		  (module_t, int, void *);

Static usbd_status ubt_request_start      (ubt_softc_p);
Static void        ubt_request_complete   (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_request_complete2  (node_p, hook_p, void *, int);

Static usbd_status ubt_intr_start	  (ubt_softc_p);
Static void        ubt_intr_complete      (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_intr_complete2     (node_p, hook_p, void *, int); 

Static usbd_status ubt_bulk_in_start	  (ubt_softc_p);
Static void        ubt_bulk_in_complete   (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_bulk_in_complete2  (node_p, hook_p, void *, int);

Static usbd_status ubt_bulk_out_start     (ubt_softc_p);
Static void        ubt_bulk_out_complete  (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_bulk_out_complete2 (node_p, hook_p, void *, int); 

Static usbd_status ubt_isoc_in_start      (ubt_softc_p);
Static void        ubt_isoc_in_complete   (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_isoc_in_complete2  (node_p, hook_p, void *, int);

Static usbd_status ubt_isoc_out_start     (ubt_softc_p);
Static void        ubt_isoc_out_complete  (usbd_xfer_handle, 
					   usbd_private_handle, usbd_status);
Static void        ubt_isoc_out_complete2 (node_p, hook_p, void *, int);

Static void        ubt_reset              (ubt_softc_p);

/*
 * Netgraph methods
 */

Static ng_constructor_t	ng_ubt_constructor;
Static ng_shutdown_t	ng_ubt_shutdown;
Static ng_newhook_t	ng_ubt_newhook;
Static ng_connect_t	ng_ubt_connect;
Static ng_disconnect_t	ng_ubt_disconnect;
Static ng_rcvmsg_t	ng_ubt_rcvmsg;
Static ng_rcvdata_t	ng_ubt_rcvdata;

/* Queue length */
Static const struct ng_parse_struct_field	ng_ubt_node_qlen_type_fields[] =
{
	{ "queue", &ng_parse_int32_type, },
	{ "qlen",  &ng_parse_int32_type, },
	{ NULL, }
};
Static const struct ng_parse_type		ng_ubt_node_qlen_type = {
	&ng_parse_struct_type,
	&ng_ubt_node_qlen_type_fields
};

/* Stat info */
Static const struct ng_parse_struct_field	ng_ubt_node_stat_type_fields[] =
{
	{ "pckts_recv", &ng_parse_uint32_type, },
	{ "bytes_recv", &ng_parse_uint32_type, },
	{ "pckts_sent", &ng_parse_uint32_type, },
	{ "bytes_sent", &ng_parse_uint32_type, },
	{ "oerrors",    &ng_parse_uint32_type, },
	{ "ierrors",    &ng_parse_uint32_type, },
	{ NULL, }
};
Static const struct ng_parse_type	ng_ubt_node_stat_type = {
	&ng_parse_struct_type,
	&ng_ubt_node_stat_type_fields
};

/* Netgraph node command list */
Static const struct ng_cmdlist	ng_ubt_cmdlist[] = {
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_SET_DEBUG,
	"set_debug",
	&ng_parse_uint16_type,
	NULL
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_GET_DEBUG,
	"get_debug",
	NULL,
	&ng_parse_uint16_type
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_SET_QLEN,
	"set_qlen",
	&ng_ubt_node_qlen_type,
	NULL
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_GET_QLEN,
	"get_qlen",
	&ng_ubt_node_qlen_type,
	&ng_ubt_node_qlen_type
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_GET_STAT,
	"get_stat",
	NULL,
	&ng_ubt_node_stat_type
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_RESET_STAT,
	"reset_stat",
        NULL,
	NULL
},
{
	NGM_UBT_COOKIE,
	NGM_UBT_NODE_DEV_NODES,
	"dev_nodes",
        &ng_parse_uint16_type,
	NULL
},
{ 0, }
};

/* Netgraph node type */
Static struct ng_type	typestruct = {
	NG_ABI_VERSION,
	NG_UBT_NODE_TYPE,	/* typename */
	NULL,			/* modevent */
	ng_ubt_constructor,	/* constructor */
	ng_ubt_rcvmsg,		/* control message */
	ng_ubt_shutdown,	/* destructor */
	ng_ubt_newhook,		/* new hook */
	NULL,			/* find hook */
	ng_ubt_connect,		/* connect hook */
	ng_ubt_rcvdata,		/* data */
	ng_ubt_disconnect,	/* disconnect hook */
	ng_ubt_cmdlist,		/* node command list */
};

/*
 * Device methods
 */

#define UBT_UNIT(n)	((minor(n) >> 4) & 0xf)
#define UBT_ENDPOINT(n)	(minor(n) & 0xf)
#define UBT_MINOR(u, e)	(((u) << 4) | (e))
#define UBT_BSIZE	1024

Static d_open_t		ubt_open;
Static d_close_t	ubt_close;
Static d_read_t		ubt_read;
Static d_write_t	ubt_write;
Static d_ioctl_t	ubt_ioctl;
Static d_poll_t		ubt_poll;
Static void		ubt_create_device_nodes  (ubt_softc_p);
Static void		ubt_destroy_device_nodes (ubt_softc_p);

#if __FreeBSD_version < 500104
#define CDEV_MAJOR	222
#else
#define CDEV_MAJOR	MAJOR_AUTO
#endif

Static struct cdevsw	ubt_cdevsw = {
	.d_open =	ubt_open,
	.d_close =	ubt_close,
	.d_read =	ubt_read,
	.d_write =	ubt_write,
	.d_ioctl =	ubt_ioctl,
	.d_poll =	ubt_poll,
	.d_name =	"ubt",
	.d_maj =	CDEV_MAJOR,
};

/*
 * Module
 */

DRIVER_MODULE(ubt, uhub, ubt_driver, ubt_devclass, ubt_modevent, 0);
MODULE_VERSION(ng_ubt, NG_BLUETOOTH_VERSION);
MODULE_DEPEND(ng_ubt, netgraph, NG_ABI_VERSION, NG_ABI_VERSION, NG_ABI_VERSION);

/****************************************************************************
 ****************************************************************************
 **                              USB specific
 ****************************************************************************
 ****************************************************************************/

/*
 * Load/Unload the driver module
 */

Static int
ubt_modevent(module_t mod, int event, void *data)
{
	int	error;

	switch (event) {
	case MOD_LOAD:
		error = ng_newtype(&typestruct);
		if (error != 0)
			printf(
"%s: Could not register Netgraph node type, error=%d\n",
				NG_UBT_NODE_TYPE, error);
		else
			error = usbd_driver_load(mod, event, data);
		break;

	case MOD_UNLOAD:
		error = ng_rmtype(&typestruct);
		if (error == 0)
			error = usbd_driver_load(mod, event, data);
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	return (error);
} /* ubt_modevent */

/*
 * Probe for a USB Bluetooth device
 */

USB_MATCH(ubt)
{
	/*
	 * If for some reason device should not be attached then put
	 * VendorID/ProductID pair into the list below. Currently I
	 * do not know of any such devices. The format is as follows:
	 *
	 *	{ VENDOR_ID, PRODUCT_ID },
	 *
	 * where VENDOR_ID and PRODUCT_ID are hex numbers.
	 */

	Static struct usb_devno const	ubt_ignored_devices[] = {
		{ 0, 0 } /* This should be the last item in the list */
	};

	/*
	 * If device violates Bluetooth specification and has bDeviceClass,
	 * bDeviceSubClass and bDeviceProtocol set to wrong values then you
	 * could try to put VendorID/ProductID pair into the list below. 
	 * Currently I do not know of any such devices.
	 */

	Static struct usb_devno const	ubt_broken_devices[] = {
		{ 0, 0 } /* This should be the last item in the list */
	};

	USB_MATCH_START(ubt, uaa);

	usb_device_descriptor_t	*dd = usbd_get_device_descriptor(uaa->device);

	if (uaa->iface == NULL ||
	    usb_lookup(ubt_ignored_devices, uaa->vendor, uaa->product))
		return (UMATCH_NONE);
	
	if (dd->bDeviceClass == UDCLASS_WIRELESS &&
	    dd->bDeviceSubClass == UDSUBCLASS_RF &&
	    dd->bDeviceProtocol == UDPROTO_BLUETOOTH)
		return (UMATCH_DEVCLASS_DEVSUBCLASS);

	if (usb_lookup(ubt_broken_devices, uaa->vendor, uaa->product))
		return (UMATCH_VENDOR_PRODUCT);

	return (UMATCH_NONE);
} /* USB_MATCH(ubt) */

/*
 * Attach the device
 */

USB_ATTACH(ubt)
{
	USB_ATTACH_START(ubt, sc, uaa);
	usb_config_descriptor_t		*cd = NULL;
	usb_interface_descriptor_t	*id = NULL;
	usb_endpoint_descriptor_t	*ed = NULL;
	char				 devinfo[UBT_BSIZE];
	usbd_status			 error;
	int				 i, ai, alt_no, isoc_in, isoc_out,
					 isoc_isize, isoc_osize;

	/* Get USB device info */
	sc->sc_udev = uaa->device;
	usbd_devinfo(sc->sc_udev, 0, devinfo); 
	USB_ATTACH_SETUP;
	printf("%s: %s\n", USBDEVNAME(sc->sc_dev), devinfo);

	/* 
	 * Initialize device softc structure
	 */

	/* State */
	sc->sc_debug = NG_UBT_WARN_LEVEL;
	sc->sc_flags = 0;
	NG_UBT_STAT_RESET(sc->sc_stat);

	/* Interfaces */
	sc->sc_iface0 = sc->sc_iface1 = NULL;

	/* Interrupt pipe */
	sc->sc_intr_ep = -1;
	sc->sc_intr_pipe = NULL;
	sc->sc_intr_xfer = NULL;
	sc->sc_intr_buffer = NULL;

	/* Control pipe */
	sc->sc_ctrl_xfer = NULL;
	sc->sc_ctrl_buffer = NULL;
	NG_BT_MBUFQ_INIT(&sc->sc_cmdq, UBT_DEFAULT_QLEN);

	/* Bulk-in pipe */
	sc->sc_bulk_in_ep = -1;
	sc->sc_bulk_in_pipe = NULL;
	sc->sc_bulk_in_xfer = NULL;
	sc->sc_bulk_in_buffer = NULL;

	/* Bulk-out pipe */
	sc->sc_bulk_out_ep = -1;
	sc->sc_bulk_out_pipe = NULL;
	sc->sc_bulk_out_xfer = NULL;
	sc->sc_bulk_out_buffer = NULL;
	NG_BT_MBUFQ_INIT(&sc->sc_aclq, UBT_DEFAULT_QLEN);

	/* Isoc-in pipe */
	sc->sc_isoc_in_ep = -1;
	sc->sc_isoc_in_pipe = NULL;
	sc->sc_isoc_in_xfer = NULL;

	/* Isoc-out pipe */
	sc->sc_isoc_out_ep = -1;
	sc->sc_isoc_out_pipe = NULL;
	sc->sc_isoc_out_xfer = NULL;
	sc->sc_isoc_size = -1;
	NG_BT_MBUFQ_INIT(&sc->sc_scoq, UBT_DEFAULT_QLEN);

	/* Netgraph part */
	sc->sc_node = NULL;
	sc->sc_hook = NULL;

	/* Device part */
	sc->sc_ctrl_dev = sc->sc_intr_dev = sc->sc_bulk_dev = NODEV;
	sc->sc_refcnt = sc->sc_dying = 0;

	/*
	 * XXX set configuration?
	 *
	 * Configure Bluetooth USB device. Discover all required USB interfaces
	 * and endpoints.
	 *
	 * USB device must present two interfaces:
	 * 1) Interface 0 that has 3 endpoints
	 *	1) Interrupt endpoint to receive HCI events
	 *	2) Bulk IN endpoint to receive ACL data
	 *	3) Bulk OUT endpoint to send ACL data
	 *
	 * 2) Interface 1 then has 2 endpoints
	 *	1) Isochronous IN endpoint to receive SCO data
 	 *	2) Isochronous OUT endpoint to send SCO data
	 *
	 * Interface 1 (with isochronous endpoints) has several alternate 
	 * configurations with different packet size.
	 */

	/*
	 * Interface 0
	 */

	error = usbd_device2interface_handle(sc->sc_udev, 0, &sc->sc_iface0);
	if (error || sc->sc_iface0 == NULL) {
		printf("%s: Could not get interface 0 handle. %s (%d), " \
			"handle=%p\n", USBDEVNAME(sc->sc_dev),
			usbd_errstr(error), error, sc->sc_iface0);
		goto bad;
	}

	id = usbd_get_interface_descriptor(sc->sc_iface0);
	if (id == NULL) {
		printf("%s: Could not get interface 0 descriptor\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	for (i = 0; i < id->bNumEndpoints; i ++) {
		ed = usbd_interface2endpoint_descriptor(sc->sc_iface0, i);
		if (ed == NULL) {
			printf("%s: Could not read endpoint descriptor for " \
				"interface 0, i=%d\n", USBDEVNAME(sc->sc_dev),
				i);
			goto bad;
		}

		switch (UE_GET_XFERTYPE(ed->bmAttributes)) {
		case UE_BULK:
			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN)
				sc->sc_bulk_in_ep = ed->bEndpointAddress;
			else
				sc->sc_bulk_out_ep = ed->bEndpointAddress;
			break;

		case UE_INTERRUPT:
			sc->sc_intr_ep = ed->bEndpointAddress;
			break;
		}
	}

	/* Check if we got everything we wanted on Interface 0 */
	if (sc->sc_intr_ep == -1) {
		printf("%s: Could not detect interrupt endpoint\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (sc->sc_bulk_in_ep == -1) {
		printf("%s: Could not detect bulk-in endpoint\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (sc->sc_bulk_out_ep == -1) {
		printf("%s: Could not detect bulk-out endpoint\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	printf("%s: Interface 0 endpoints: interrupt=%#x, bulk-in=%#x, " \
		"bulk-out=%#x\n", USBDEVNAME(sc->sc_dev), 
		sc->sc_intr_ep, sc->sc_bulk_in_ep, sc->sc_bulk_out_ep);

	/*
	 * Interface 1
	 */

	cd = usbd_get_config_descriptor(sc->sc_udev);
	if (cd == NULL) {
		printf("%s: Could not get device configuration descriptor\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	error = usbd_device2interface_handle(sc->sc_udev, 1, &sc->sc_iface1);
	if (error || sc->sc_iface1 == NULL) {
		printf("%s: Could not get interface 1 handle. %s (%d), " \
			"handle=%p\n", USBDEVNAME(sc->sc_dev), 
			usbd_errstr(error), error, sc->sc_iface1);
		goto bad;
	}

	id = usbd_get_interface_descriptor(sc->sc_iface1);
	if (id == NULL) {
		printf("%s: Could not get interface 1 descriptor\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	/*
	 * Scan all alternate configurations for interface 1
	 */

	alt_no = -1;

	for (ai = 0; ai < usbd_get_no_alts(cd, 1); ai++)  {
		error = usbd_set_interface(sc->sc_iface1, ai);
		if (error) {
			printf("%s: [SCAN] Could not set alternate " \
				"configuration %d for interface 1. %s (%d)\n",
				USBDEVNAME(sc->sc_dev),  ai, usbd_errstr(error),
				error);
			goto bad;
		}
		id = usbd_get_interface_descriptor(sc->sc_iface1);
		if (id == NULL) {
			printf("%s: Could not get interface 1 descriptor for " \
				"alternate configuration %d\n",
				USBDEVNAME(sc->sc_dev), ai);
			goto bad;
		}

		isoc_in = isoc_out = -1;
		isoc_isize = isoc_osize = 0;

		for (i = 0; i < id->bNumEndpoints; i ++) {
			ed = usbd_interface2endpoint_descriptor(sc->sc_iface1, i);
			if (ed == NULL) {
				printf("%s: Could not read endpoint " \
					"descriptor for interface 1, " \
					"alternate configuration %d, i=%d\n",
					USBDEVNAME(sc->sc_dev), ai, i);
				goto bad;
			}

			if (UE_GET_XFERTYPE(ed->bmAttributes) != UE_ISOCHRONOUS)
				continue;

			if (UE_GET_DIR(ed->bEndpointAddress) == UE_DIR_IN) {
				isoc_in = ed->bEndpointAddress;
				isoc_isize = UGETW(ed->wMaxPacketSize);
			} else {
				isoc_out = ed->bEndpointAddress;
				isoc_osize = UGETW(ed->wMaxPacketSize);
			}
		}

		/*
		 * Make sure that configuration looks sane and if so
		 * update current settings
		 */

		if (isoc_in != -1 && isoc_out != -1 &&
		    isoc_isize > 0  && isoc_osize > 0 &&
		    isoc_isize == isoc_osize && isoc_isize > sc->sc_isoc_size) {
			sc->sc_isoc_in_ep = isoc_in;
			sc->sc_isoc_out_ep = isoc_out;
			sc->sc_isoc_size = isoc_isize;
			alt_no = ai;
		}
	}

	/* Check if we got everything we wanted on Interface 0 */
	if (sc->sc_isoc_in_ep == -1) {
		printf("%s: Could not detect isoc-in endpoint\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (sc->sc_isoc_out_ep == -1) {
		printf("%s: Could not detect isoc-out endpoint\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	if (sc->sc_isoc_size <= 0) {
		printf("%s: Invalid isoc. packet size=%d\n",
			USBDEVNAME(sc->sc_dev), sc->sc_isoc_size);
		goto bad;
	}

	error = usbd_set_interface(sc->sc_iface1, alt_no);
	if (error) {
		printf("%s: Could not set alternate configuration " \
			"%d for interface 1. %s (%d)\n", USBDEVNAME(sc->sc_dev),
			alt_no, usbd_errstr(error), error);
		goto bad;
	}

	/* Allocate USB transfer handles and buffers */
	sc->sc_ctrl_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_ctrl_xfer == NULL) {
		printf("%s: Could not allocate control xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_ctrl_buffer = usbd_alloc_buffer(sc->sc_ctrl_xfer, 
						UBT_CTRL_BUFFER_SIZE);
	if (sc->sc_ctrl_buffer == NULL) {
		printf("%s: Could not allocate control buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	sc->sc_intr_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_intr_xfer == NULL) {
		printf("%s: Could not allocate interrupt xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	sc->sc_bulk_in_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_bulk_in_xfer == NULL) {
		printf("%s: Could not allocate bulk-in xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	sc->sc_bulk_out_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_bulk_out_xfer == NULL) {
		printf("%s: Could not allocate bulk-out xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_bulk_out_buffer = usbd_alloc_buffer(sc->sc_bulk_out_xfer,
						UBT_BULK_BUFFER_SIZE);
	if (sc->sc_bulk_out_buffer == NULL) {
		printf("%s: Could not allocate bulk-out buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	/*
	 * Allocate buffers for isoc. transfers
	 */

	sc->sc_isoc_nframes = (UBT_ISOC_BUFFER_SIZE / sc->sc_isoc_size) + 1;

	sc->sc_isoc_in_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_isoc_in_xfer == NULL) {
		printf("%s: Could not allocate isoc-in xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_isoc_in_buffer = usbd_alloc_buffer(sc->sc_isoc_in_xfer,
					sc->sc_isoc_nframes * sc->sc_isoc_size);
	if (sc->sc_isoc_in_buffer == NULL) {
		printf("%s: Could not allocate isoc-in buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_isoc_in_frlen = malloc(sizeof(u_int16_t) * sc->sc_isoc_nframes, 
						M_USBDEV, M_NOWAIT);
	if (sc->sc_isoc_in_frlen == NULL) {
		printf("%s: Could not allocate isoc-in frame sizes buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	sc->sc_isoc_out_xfer = usbd_alloc_xfer(sc->sc_udev);
	if (sc->sc_isoc_out_xfer == NULL) {
		printf("%s: Could not allocate isoc-out xfer handle\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_isoc_out_buffer = usbd_alloc_buffer(sc->sc_isoc_out_xfer,
					sc->sc_isoc_nframes * sc->sc_isoc_size);
	if (sc->sc_isoc_out_buffer == NULL) {
		printf("%s: Could not allocate isoc-out buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}
	sc->sc_isoc_out_frlen = malloc(sizeof(u_int16_t) * sc->sc_isoc_nframes, 
						M_USBDEV, M_NOWAIT);
	if (sc->sc_isoc_out_frlen == NULL) {
		printf("%s: Could not allocate isoc-out frame sizes buffer\n",
			USBDEVNAME(sc->sc_dev));
		goto bad;
	}

	printf("%s: Interface 1 (alt.config %d) endpoints: isoc-in=%#x, " \
		"isoc-out=%#x; wMaxPacketSize=%d; nframes=%d, buffer size=%d\n",
		USBDEVNAME(sc->sc_dev), alt_no, sc->sc_isoc_in_ep,
		sc->sc_isoc_out_ep, sc->sc_isoc_size, sc->sc_isoc_nframes, 
		(sc->sc_isoc_nframes * sc->sc_isoc_size));

	/*
	 * Open pipes
	 */

	/* Interrupt */	
	error = usbd_open_pipe(sc->sc_iface0, sc->sc_intr_ep,
			USBD_EXCLUSIVE_USE, &sc->sc_intr_pipe);
	if (error != USBD_NORMAL_COMPLETION) {
		printf("%s: %s - Could not open interrupt pipe. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(error),
			error);
		goto bad;
	}

	/* Bulk-in */
	error = usbd_open_pipe(sc->sc_iface0, sc->sc_bulk_in_ep,
			USBD_EXCLUSIVE_USE, &sc->sc_bulk_in_pipe);
	if (error != USBD_NORMAL_COMPLETION) {
		printf("%s: %s - Could not open bulk-in pipe. %s (%d)\n",
			__func__,  USBDEVNAME(sc->sc_dev), usbd_errstr(error),
			error);
		goto bad;
	}

	/* Bulk-out */
	error = usbd_open_pipe(sc->sc_iface0, sc->sc_bulk_out_ep,
			USBD_EXCLUSIVE_USE, &sc->sc_bulk_out_pipe);
	if (error != USBD_NORMAL_COMPLETION) {
		printf("%s: %s - Could not open bulk-out pipe. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(error),
			error);
		goto bad;
	}

#if __broken__ /* XXX FIXME */
	/* Isoc-in */
	error = usbd_open_pipe(sc->sc_iface1, sc->sc_isoc_in_ep,
			USBD_EXCLUSIVE_USE, &sc->sc_isoc_in_pipe);
	if (error != USBD_NORMAL_COMPLETION) {
		printf("%s: %s - Could not open isoc-in pipe. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(error),
			error);
		goto bad;
	}

	/* Isoc-out */
	error = usbd_open_pipe(sc->sc_iface1, sc->sc_isoc_out_ep, 
			USBD_EXCLUSIVE_USE, &sc->sc_isoc_out_pipe);
	if (error != USBD_NORMAL_COMPLETION) {
		printf("%s: %s - Could not open isoc-out pipe. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(error),
			error);
		goto bad;
	}
#endif /* __broken__ */

	/* Create Netgraph node */
	if (ng_make_node_common(&typestruct, &sc->sc_node) != 0) {
		printf("%s: Could not create Netgraph node\n",
			USBDEVNAME(sc->sc_dev));
		sc->sc_node = NULL;
		goto bad;
	}

	/* Name node */
	if (ng_name_node(sc->sc_node, USBDEVNAME(sc->sc_dev)) != 0) {
		printf("%s: Could not name Netgraph node\n",
			USBDEVNAME(sc->sc_dev));
		NG_NODE_UNREF(sc->sc_node);
		sc->sc_node = NULL;
		goto bad;
	}

	NG_NODE_SET_PRIVATE(sc->sc_node, sc);
	NG_NODE_FORCE_WRITER(sc->sc_node);

	/* Claim all interfaces on the device */
	for (i = 0; i < uaa->nifaces; i++)
		uaa->ifaces[i] = NULL;

	usbd_add_drv_event(USB_EVENT_DRIVER_ATTACH, sc->sc_udev,
		USBDEV(sc->sc_dev));

	USB_ATTACH_SUCCESS_RETURN;
bad:
	ubt_detach(self);

	USB_ATTACH_ERROR_RETURN;
} /* USB_ATTACH(ubt) */

/*
 * Detach the device
 */

USB_DETACH(ubt)
{
	USB_DETACH_START(ubt, sc);

	sc->sc_dying = 1;

	ubt_destroy_device_nodes(sc); /* XXX FIXME locking? */

	/* Destroy Netgraph node */
	if (sc->sc_node != NULL) {
		NG_NODE_SET_PRIVATE(sc->sc_node, NULL);
		ng_rmnode_self(sc->sc_node);
		sc->sc_node = NULL;
	}

	/* Close pipes */
	if (sc->sc_intr_pipe != NULL) {
		usbd_close_pipe(sc->sc_intr_pipe);
		sc->sc_intr_pipe = NULL;
	}

	if (sc->sc_bulk_in_pipe != NULL) {
		usbd_close_pipe(sc->sc_bulk_in_pipe);
		sc->sc_bulk_in_pipe = NULL;
	}
	if (sc->sc_bulk_out_pipe != NULL) {
		usbd_close_pipe(sc->sc_bulk_out_pipe);
		sc->sc_bulk_out_pipe = NULL;
	}

	if (sc->sc_isoc_in_pipe != NULL) {
		usbd_close_pipe(sc->sc_isoc_in_pipe);
		sc->sc_isoc_in_pipe = NULL;
	}
	if (sc->sc_isoc_out_pipe != NULL) {
		usbd_close_pipe(sc->sc_isoc_out_pipe);
		sc->sc_isoc_out_pipe = NULL;
	}

	/* Destroy USB transfer handles */
	if (sc->sc_ctrl_xfer != NULL) {
		usbd_free_xfer(sc->sc_ctrl_xfer);
		sc->sc_ctrl_xfer = NULL;
	}

	if (sc->sc_intr_xfer != NULL) {
		usbd_free_xfer(sc->sc_intr_xfer);
		sc->sc_intr_xfer = NULL;
	}

	if (sc->sc_bulk_in_xfer != NULL) {
		usbd_free_xfer(sc->sc_bulk_in_xfer);
		sc->sc_bulk_in_xfer = NULL;
	}
	if (sc->sc_bulk_out_xfer != NULL) {
		usbd_free_xfer(sc->sc_bulk_out_xfer);
		sc->sc_bulk_out_xfer = NULL;
	}

	if (sc->sc_isoc_in_xfer != NULL) {
		usbd_free_xfer(sc->sc_isoc_in_xfer);
		sc->sc_isoc_in_xfer = NULL;
	}
	if (sc->sc_isoc_out_xfer != NULL) {
		usbd_free_xfer(sc->sc_isoc_out_xfer);
		sc->sc_isoc_out_xfer = NULL;
	}

	/* Destroy isoc. frame size buffers */
	if (sc->sc_isoc_in_frlen != NULL) {
		free(sc->sc_isoc_in_frlen, M_USBDEV);
		sc->sc_isoc_in_frlen = NULL;
	}
	if (sc->sc_isoc_out_frlen != NULL) {
		free(sc->sc_isoc_out_frlen, M_USBDEV);
		sc->sc_isoc_out_frlen = NULL;
	}

	/* Destroy queues */
	NG_BT_MBUFQ_DRAIN(&sc->sc_cmdq);
	NG_BT_MBUFQ_DRAIN(&sc->sc_aclq);
	NG_BT_MBUFQ_DRAIN(&sc->sc_scoq);

	usbd_add_drv_event(USB_EVENT_DRIVER_DETACH, sc->sc_udev,
			USBDEV(sc->sc_dev));

	return (0);
} /* USB_DETACH(ubt) */

/*
 * Start USB control request (HCI command). Must be called with node locked
 */

Static usbd_status
ubt_request_start(ubt_softc_p sc)
{
	usb_device_request_t	 req;
	struct mbuf		*m = NULL;
	usbd_status		 status;

	KASSERT(!(sc->sc_flags & UBT_CMD_XMIT), (
"%s: %s - Another control request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	NG_BT_MBUFQ_DEQUEUE(&sc->sc_cmdq, m);
	if (m == NULL) {
		NG_UBT_INFO(
"%s: %s - HCI command queue is empty\n", __func__, USBDEVNAME(sc->sc_dev));

		return (USBD_NORMAL_COMPLETION);
	}

	/*
	 * Check HCI command frame size and copy it back to 
	 * linear USB transfer buffer.
	 */ 

	if (m->m_pkthdr.len > UBT_CTRL_BUFFER_SIZE)
		panic(
"%s: %s - HCI command frame too big, size=%d, len=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), UBT_CTRL_BUFFER_SIZE,
			m->m_pkthdr.len);

	m_copydata(m, 0, m->m_pkthdr.len, sc->sc_ctrl_buffer);

	/* Initialize a USB control request and then schedule it */
	bzero(&req, sizeof(req));
	req.bmRequestType = UBT_HCI_REQUEST;
	USETW(req.wLength, m->m_pkthdr.len);

	NG_UBT_INFO(
"%s: %s - Sending control request, bmRequestType=%#x, wLength=%d\n",
		__func__, USBDEVNAME(sc->sc_dev), req.bmRequestType,
		UGETW(req.wLength));

	usbd_setup_default_xfer(
		sc->sc_ctrl_xfer,
		sc->sc_udev,
		(usbd_private_handle) sc->sc_node,
		USBD_DEFAULT_TIMEOUT, /* XXX */
		&req,
		sc->sc_ctrl_buffer,
		m->m_pkthdr.len,
		USBD_NO_COPY,
		ubt_request_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_ctrl_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Could not start control request. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev),
			usbd_errstr(status), status);

		NG_NODE_UNREF(sc->sc_node);

		NG_BT_MBUFQ_DROP(&sc->sc_cmdq);
		NG_UBT_STAT_OERROR(sc->sc_stat);

		/* XXX FIXME should we try to resubmit another request? */
	} else {
		NG_UBT_INFO(
"%s: %s - Control request has been started\n",
			__func__, USBDEVNAME(sc->sc_dev));

		sc->sc_flags |= UBT_CMD_XMIT;
		status = USBD_NORMAL_COMPLETION;
	}

	NG_FREE_M(m);

	return (status);
} /* ubt_request_start */

/*
 * USB control request callback
 */

Static void
ubt_request_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p, NULL, ubt_request_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_request_complete */

Static void
ubt_request_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{ 
	ubt_softc_p		sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	h = (usbd_xfer_handle) arg1;
	usbd_status		s = (usbd_status) arg2;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_CMD_XMIT), (
"%s: %s - No control request is pending\n", __func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_CMD_XMIT;

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Control request cancelled\n", __func__, USBDEVNAME(sc->sc_dev));

		return;
	}

	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_ERR(
"%s: %s - Control request failed. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(h->pipe);

		NG_UBT_STAT_OERROR(sc->sc_stat);
	} else {
		NG_UBT_INFO(
"%s: %s - Sent %d bytes to control pipe\n",
			__func__, USBDEVNAME(sc->sc_dev), h->actlen);

		NG_UBT_STAT_BYTES_SENT(sc->sc_stat, h->actlen);
		NG_UBT_STAT_PCKTS_SENT(sc->sc_stat);
	}

	if (NG_BT_MBUFQ_LEN(&sc->sc_cmdq) > 0)
		ubt_request_start(sc);
} /* ubt_request_complete2 */

/*
 * Start interrupt transfer. Must be called when node is locked
 */

Static usbd_status
ubt_intr_start(ubt_softc_p sc)
{
	struct mbuf	*m = NULL;
	usbd_status	 status;

	KASSERT(!(sc->sc_flags & UBT_EVT_RECV), (
"%s: %s - Another interrupt request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	/* Allocate new mbuf cluster */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (USBD_NOMEM);

	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		NG_FREE_M(m);
		return (USBD_NOMEM);
	}

	if (!(sc->sc_flags & UBT_HAVE_FRAME_TYPE)) {
		*mtod(m, u_int8_t *) = NG_HCI_EVENT_PKT;
		m->m_pkthdr.len = m->m_len = 1;
	} else
		m->m_pkthdr.len = m->m_len = 0;
	
	/* Initialize a USB transfer and then schedule it */
	usbd_setup_xfer(
			sc->sc_intr_xfer,
			sc->sc_intr_pipe,
			(usbd_private_handle) sc->sc_node,
			(void *)(mtod(m, u_int8_t *) + m->m_len),
			MCLBYTES - m->m_len,
			USBD_SHORT_XFER_OK,
			USBD_NO_TIMEOUT,
			ubt_intr_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_intr_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Failed to start intrerrupt transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);

		NG_NODE_UNREF(sc->sc_node);

		NG_FREE_M(m);

		return (status);
	}

	sc->sc_flags |= UBT_EVT_RECV;
	sc->sc_intr_buffer = m;

	return (USBD_NORMAL_COMPLETION);
} /* ubt_intr_start */

/*
 * Process interrupt from USB device (We got data from interrupt pipe)
 */

Static void
ubt_intr_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p, NULL, ubt_intr_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_intr_complete */

Static void
ubt_intr_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{
	ubt_softc_p		 sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	 h = (usbd_xfer_handle) arg1;
	usbd_status		 s = (usbd_status) arg2;
	struct mbuf		*m = NULL;
	ng_hci_event_pkt_t	*hdr = NULL;
	int			 error;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_EVT_RECV), (
"%s: %s - No interrupt request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_EVT_RECV;

	m = sc->sc_intr_buffer;
	sc->sc_intr_buffer = NULL;

	hdr = mtod(m, ng_hci_event_pkt_t *);

	if (sc->sc_hook == NULL || NG_HOOK_NOT_VALID(sc->sc_hook)) {
		NG_UBT_INFO(
"%s: %s - No upstream hook\n", __func__, USBDEVNAME(sc->sc_dev));

		NG_FREE_M(m);
		return;
	}

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Interrupt xfer cancelled\n", __func__, USBDEVNAME(sc->sc_dev));

		NG_FREE_M(m);
		return;
	}
		
	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_WARN(
"%s: %s - Interrupt xfer failed, %s (%d). No new xfer will be submitted!\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_intr_pipe);

		NG_UBT_STAT_IERROR(sc->sc_stat);
		NG_FREE_M(m);

		return; /* XXX FIXME we should restart after some delay */
	}

	NG_UBT_STAT_BYTES_RECV(sc->sc_stat, h->actlen);
	m->m_pkthdr.len += h->actlen;
	m->m_len += h->actlen;

	NG_UBT_INFO(
"%s: %s - Got %d bytes from interrupt pipe\n",
		__func__, USBDEVNAME(sc->sc_dev), h->actlen);

	if (m->m_pkthdr.len < sizeof(*hdr)) {
		NG_FREE_M(m);
		goto done;
	}

	if (hdr->length == m->m_pkthdr.len - sizeof(*hdr)) {
		NG_UBT_INFO(
"%s: %s - Got complete HCI event frame, pktlen=%d, length=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), m->m_pkthdr.len,
			hdr->length);

		NG_UBT_STAT_PCKTS_RECV(sc->sc_stat);

		NG_SEND_DATA_ONLY(error, sc->sc_hook, m);
		if (error != 0)
			NG_UBT_STAT_IERROR(sc->sc_stat);
	} else {
		NG_UBT_ERR(
"%s: %s - Invalid HCI event frame size, length=%d, pktlen=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), hdr->length, 
			m->m_pkthdr.len);

		NG_UBT_STAT_IERROR(sc->sc_stat);
		NG_FREE_M(m);
	}
done:
	ubt_intr_start(sc);
} /* ubt_intr_complete2 */

/*
 * Start bulk-in USB transfer (ACL data). Must be called when node is locked
 */

Static usbd_status
ubt_bulk_in_start(ubt_softc_p sc)
{
	struct mbuf	*m = NULL;
	usbd_status	 status;

	KASSERT(!(sc->sc_flags & UBT_ACL_RECV), (
"%s: %s - Another bulk-in request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	/* Allocate new mbuf cluster */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL)
		return (USBD_NOMEM);

	MCLGET(m, M_DONTWAIT);
	if (!(m->m_flags & M_EXT)) {
		NG_FREE_M(m);
		return (USBD_NOMEM);
	}

	if (!(sc->sc_flags & UBT_HAVE_FRAME_TYPE)) {
		*mtod(m, u_int8_t *) = NG_HCI_ACL_DATA_PKT;
		m->m_pkthdr.len = m->m_len = 1;
	} else
		m->m_pkthdr.len = m->m_len = 0;
	
	/* Initialize a bulk-in USB transfer and then schedule it */
	usbd_setup_xfer(
			sc->sc_bulk_in_xfer,
			sc->sc_bulk_in_pipe,
			(usbd_private_handle) sc->sc_node,
			(void *)(mtod(m, u_int8_t *) + m->m_len),
			MCLBYTES - m->m_len,
			USBD_SHORT_XFER_OK,
			USBD_NO_TIMEOUT,
			ubt_bulk_in_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_bulk_in_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Failed to start bulk-in transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);

		NG_NODE_UNREF(sc->sc_node);

		NG_FREE_M(m);

		return (status);
	}

	sc->sc_flags |= UBT_ACL_RECV;
	sc->sc_bulk_in_buffer = m;

	return (USBD_NORMAL_COMPLETION);
} /* ubt_bulk_in_start */

/*
 * USB bulk-in transfer callback
 */

Static void
ubt_bulk_in_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p, NULL, ubt_bulk_in_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_bulk_in_complete */

Static void
ubt_bulk_in_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{
	ubt_softc_p		 sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	 h = (usbd_xfer_handle) arg1;
	usbd_status		 s = (usbd_status) arg2;
	struct mbuf		*m = NULL;
	ng_hci_acldata_pkt_t	*hdr = NULL;
	int			 len;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_ACL_RECV), (
"%s: %s - No bulk-in request is pending\n", __func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_ACL_RECV;

	m = sc->sc_bulk_in_buffer;
	sc->sc_bulk_in_buffer = NULL;

	hdr = mtod(m, ng_hci_acldata_pkt_t *);

	if (sc->sc_hook == NULL || NG_HOOK_NOT_VALID(sc->sc_hook)) {
		NG_UBT_INFO(
"%s: %s - No upstream hook\n", __func__, USBDEVNAME(sc->sc_dev));

		NG_FREE_M(m);
		return;
	}

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Bulk-in xfer cancelled, pipe=%p\n",
			__func__, USBDEVNAME(sc->sc_dev), sc->sc_bulk_in_pipe);

		NG_FREE_M(m);
		return;
	}

	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_WARN(
"%s: %s - Bulk-in xfer failed, %s (%d). No new xfer will be submitted!\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_bulk_in_pipe);

		NG_UBT_STAT_IERROR(sc->sc_stat);
		NG_FREE_M(m);

		return; /* XXX FIXME we should restart after some delay */
	}

	NG_UBT_STAT_BYTES_RECV(sc->sc_stat, h->actlen);
	m->m_pkthdr.len += h->actlen;
	m->m_len += h->actlen;

	NG_UBT_INFO(
"%s: %s - Got %d bytes from bulk-in pipe\n",
		__func__, USBDEVNAME(sc->sc_dev), h->actlen);

	if (m->m_pkthdr.len < sizeof(*hdr)) {
		NG_FREE_M(m);
		goto done;
	}

	len = le16toh(hdr->length);
	if (len == m->m_pkthdr.len - sizeof(*hdr)) {
		NG_UBT_INFO(
"%s: %s - Got complete ACL data frame, pktlen=%d, length=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), m->m_pkthdr.len, len);

		NG_UBT_STAT_PCKTS_RECV(sc->sc_stat);

		NG_SEND_DATA_ONLY(len, sc->sc_hook, m);
		if (len != 0)
			NG_UBT_STAT_IERROR(sc->sc_stat);
	} else {
		NG_UBT_ERR(
"%s: %s - Invalid ACL frame size, length=%d, pktlen=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), len,
			m->m_pkthdr.len);

		NG_UBT_STAT_IERROR(sc->sc_stat);
		NG_FREE_M(m);
	}
done:
	ubt_bulk_in_start(sc);
} /* ubt_bulk_in_complete2 */

/*
 * Start bulk-out USB transfer. Must be called with node locked
 */

Static usbd_status
ubt_bulk_out_start(ubt_softc_p sc)
{
	struct mbuf	*m = NULL;
	usbd_status	status;

	KASSERT(!(sc->sc_flags & UBT_ACL_XMIT), (
"%s: %s - Another bulk-out request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	NG_BT_MBUFQ_DEQUEUE(&sc->sc_aclq, m);
	if (m == NULL) {
		NG_UBT_INFO(
"%s: %s - ACL data queue is empty\n", __func__, USBDEVNAME(sc->sc_dev));

 		return (USBD_NORMAL_COMPLETION);
	}

	/*
	 * Check ACL data frame size and copy it back to linear USB 
	 * transfer buffer.
	 */ 

	if (m->m_pkthdr.len > UBT_BULK_BUFFER_SIZE)
		panic(
"%s: %s - ACL data frame too big, size=%d, len=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), UBT_BULK_BUFFER_SIZE,
			m->m_pkthdr.len);

	m_copydata(m, 0, m->m_pkthdr.len, sc->sc_bulk_out_buffer);

	/* Initialize a bulk-out USB transfer and then schedule it */
	usbd_setup_xfer(
			sc->sc_bulk_out_xfer,
			sc->sc_bulk_out_pipe,
			(usbd_private_handle) sc->sc_node,
			sc->sc_bulk_out_buffer,
			m->m_pkthdr.len,
			USBD_NO_COPY,
			USBD_DEFAULT_TIMEOUT, /* XXX */
			ubt_bulk_out_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_bulk_out_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Could not start bulk-out transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);

		NG_NODE_UNREF(sc->sc_node);

		NG_BT_MBUFQ_DROP(&sc->sc_aclq);
		NG_UBT_STAT_OERROR(sc->sc_stat);

		/* XXX FIXME should we try to start another transfer? */
	} else {
		NG_UBT_INFO(
"%s: %s - Bulk-out transfer has been started, len=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), m->m_pkthdr.len);

		sc->sc_flags |= UBT_ACL_XMIT;
		status = USBD_NORMAL_COMPLETION;
	}

	NG_FREE_M(m);

	return (status);
} /* ubt_bulk_out_start */

/*
 * USB bulk-out transfer callback
 */

Static void
ubt_bulk_out_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p,  NULL, ubt_bulk_out_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_bulk_out_complete */

Static void
ubt_bulk_out_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{
	ubt_softc_p		sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	h = (usbd_xfer_handle) arg1;
	usbd_status		s = (usbd_status) arg2;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_ACL_XMIT), (
"%s: %s - No bulk-out request is pending\n", __func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_ACL_XMIT;

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Bulk-out xfer cancelled, pipe=%p\n",
			__func__, USBDEVNAME(sc->sc_dev), sc->sc_bulk_out_pipe);

		return;
	}

	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_WARN(
"%s: %s - Bulk-out xfer failed. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_bulk_out_pipe);

		NG_UBT_STAT_OERROR(sc->sc_stat);
	} else {
		NG_UBT_INFO(
"%s: %s - Sent %d bytes to bulk-out pipe\n",
			__func__, USBDEVNAME(sc->sc_dev), h->actlen);

		NG_UBT_STAT_BYTES_SENT(sc->sc_stat, h->actlen);
		NG_UBT_STAT_PCKTS_SENT(sc->sc_stat); 
	}

	if (NG_BT_MBUFQ_LEN(&sc->sc_aclq) > 0)
		ubt_bulk_out_start(sc);
} /* ubt_bulk_out_complete2 */

/*
 * Start Isochronous-in USB transfer. Must be called with node locked
 */

Static usbd_status
ubt_isoc_in_start(ubt_softc_p sc)
{
	usbd_status	status;
	int		i;

	KASSERT(!(sc->sc_flags & UBT_SCO_RECV), (
"%s: %s - Another isoc-in request is pending\n",
                __func__, USBDEVNAME(sc->sc_dev)));

	/* Initialize a isoc-in USB transfer and then schedule it */
	for (i = 0; i < sc->sc_isoc_nframes; i++)
		sc->sc_isoc_in_frlen[i] = sc->sc_isoc_size;

	usbd_setup_isoc_xfer(
			sc->sc_isoc_in_xfer,
			sc->sc_isoc_in_pipe,
			(usbd_private_handle) sc->sc_node,
			sc->sc_isoc_in_frlen,
			sc->sc_isoc_nframes,
			USBD_NO_COPY, /* XXX flags */
			ubt_isoc_in_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_isoc_in_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Failed to start isoc-in transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev),
			usbd_errstr(status), status);

		NG_NODE_UNREF(sc->sc_node);

		return (status);
	}

	sc->sc_flags |= UBT_SCO_RECV;

	return (USBD_NORMAL_COMPLETION);
} /* ubt_isoc_in_start */

/*
 * USB isochronous transfer callback
 */

Static void
ubt_isoc_in_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p, NULL, ubt_isoc_in_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_isoc_in_complete */

Static void
ubt_isoc_in_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{
	ubt_softc_p		 sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	 h = (usbd_xfer_handle) arg1;
	usbd_status		 s = (usbd_status) arg2;
	struct mbuf		*m = NULL;
	ng_hci_scodata_pkt_t	*hdr = NULL;
	u_int8_t		*b = NULL;
	int			 i;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_SCO_RECV), (
"%s: %s - No isoc-in request is pending\n", __func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_SCO_RECV;

	if (sc->sc_hook == NULL || NG_HOOK_NOT_VALID(sc->sc_hook)) {
		NG_UBT_INFO(
"%s: %s - No upstream hook\n", __func__, USBDEVNAME(sc->sc_dev));

		return;
	}

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Isoc-in xfer cancelled, pipe=%p\n",
			__func__, USBDEVNAME(sc->sc_dev), sc->sc_isoc_in_pipe);

		return;
	}

	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_WARN(
"%s: %s - Isoc-in xfer failed, %s (%d). No new xfer will be submitted!\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_isoc_in_pipe);

		NG_UBT_STAT_IERROR(sc->sc_stat);

		return; /* XXX FIXME we should restart after some delay */
	}

	NG_UBT_STAT_BYTES_RECV(sc->sc_stat, h->actlen);

	NG_UBT_INFO(
"%s: %s - Got %d bytes from isoc-in pipe\n",
		__func__, USBDEVNAME(sc->sc_dev), h->actlen);

	/* Copy SCO data frame to mbuf */
	MGETHDR(m, M_DONTWAIT, MT_DATA);
	if (m == NULL) {
		NG_UBT_ALERT(
"%s: %s - Could not allocate mbuf\n",
			__func__, USBDEVNAME(sc->sc_dev));

		NG_UBT_STAT_IERROR(sc->sc_stat);
		goto done;
	}

	/* Fix SCO data frame header if required */
	if (!(sc->sc_flags & UBT_HAVE_FRAME_TYPE)) {
		*mtod(m, u_int8_t *) = NG_HCI_SCO_DATA_PKT;
		m->m_pkthdr.len = 1;
		m->m_len = min(MHLEN, h->actlen + 1); /* XXX m_copyback */
	} else {
		m->m_pkthdr.len = 0;
		m->m_len = min(MHLEN, h->actlen); /* XXX m_copyback */
	}

	/*
	 * XXX FIXME how do we know how many frames we have received?
	 * XXX use frlen for now. is that correct?
	 */

	b = (u_int8_t *) sc->sc_isoc_in_buffer;

	for (i = 0; i < sc->sc_isoc_nframes; i++) {
		b += (i * sc->sc_isoc_size);

		if (sc->sc_isoc_in_frlen[i] > 0)
			m_copyback(m, m->m_pkthdr.len,
				sc->sc_isoc_in_frlen[i], b);
	}

	if (m->m_pkthdr.len < sizeof(*hdr))
		goto done;

	hdr = mtod(m, ng_hci_scodata_pkt_t *);

	if (hdr->length == m->m_pkthdr.len - sizeof(*hdr)) {
		NG_UBT_INFO(
"%s: %s - Got complete SCO data frame, pktlen=%d, length=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), m->m_pkthdr.len,
			hdr->length);

		NG_UBT_STAT_PCKTS_RECV(sc->sc_stat);

		NG_SEND_DATA_ONLY(i, sc->sc_hook, m);
		if (i != 0)
			NG_UBT_STAT_IERROR(sc->sc_stat);
	} else {
		NG_UBT_ERR(
"%s: %s - Invalid SCO frame size, length=%d, pktlen=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), hdr->length, 
			m->m_pkthdr.len);

		NG_UBT_STAT_IERROR(sc->sc_stat);
		NG_FREE_M(m);
	}
done:
	ubt_isoc_in_start(sc);
} /* ubt_isoc_in_complete2 */

/*
 * Start isochronous-out USB transfer. Must be called with node locked
 */

Static usbd_status
ubt_isoc_out_start(ubt_softc_p sc)
{
	struct mbuf	*m = NULL;
	u_int8_t	*b = NULL;
	int		 i, len, nframes;
	usbd_status	 status;

	KASSERT(!(sc->sc_flags & UBT_SCO_XMIT), (
"%s: %s - Another isoc-out request is pending\n",
		__func__, USBDEVNAME(sc->sc_dev)));

	NG_BT_MBUFQ_DEQUEUE(&sc->sc_scoq, m);
	if (m == NULL) {
		NG_UBT_INFO(
"%s: %s - SCO data queue is empty\n", __func__, USBDEVNAME(sc->sc_dev));

 		return (USBD_NORMAL_COMPLETION);
	}

	/* Copy entire SCO frame into USB transfer buffer and start transfer */ 
	b = (u_int8_t *) sc->sc_isoc_out_buffer;
	nframes = 0;

	for (i = 0; i < sc->sc_isoc_nframes; i++) {
		b += (i * sc->sc_isoc_size);

		len = min(m->m_pkthdr.len, sc->sc_isoc_size);
		if (len > 0) {
			m_copydata(m, 0, len, b);
			m_adj(m, len);
			nframes ++;
		}

		sc->sc_isoc_out_frlen[i] = len;
	}

	if (m->m_pkthdr.len > 0)
		panic(
"%s: %s - SCO data frame is too big, nframes=%d, size=%d, len=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), sc->sc_isoc_nframes,
			sc->sc_isoc_size, m->m_pkthdr.len);

	NG_FREE_M(m);

	/* Initialize a isoc-out USB transfer and then schedule it */
	usbd_setup_isoc_xfer(
			sc->sc_isoc_out_xfer,
			sc->sc_isoc_out_pipe,
			(usbd_private_handle) sc->sc_node,
			sc->sc_isoc_out_frlen,
			nframes,
			USBD_NO_COPY,
			ubt_isoc_out_complete);

	NG_NODE_REF(sc->sc_node);

	status = usbd_transfer(sc->sc_isoc_out_xfer);
	if (status != USBD_NORMAL_COMPLETION && status != USBD_IN_PROGRESS) {
		NG_UBT_ERR(
"%s: %s - Could not start isoc-out transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);

		NG_NODE_UNREF(sc->sc_node);

		NG_BT_MBUFQ_DROP(&sc->sc_scoq);
		NG_UBT_STAT_OERROR(sc->sc_stat);
	} else {
		NG_UBT_INFO(
"%s: %s - Isoc-out transfer has been started, nframes=%d, size=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), nframes,
			sc->sc_isoc_size);

		sc->sc_flags |= UBT_SCO_XMIT;
		status = USBD_NORMAL_COMPLETION;
	}

	return (status);
} /* ubt_isoc_out_start */

/*
 * USB isoc-out. transfer callback
 */

Static void
ubt_isoc_out_complete(usbd_xfer_handle h, usbd_private_handle p, usbd_status s)
{
	ng_send_fn((node_p) p, NULL, ubt_isoc_out_complete2, (void *) h, s);
	NG_NODE_UNREF((node_p) p);
} /* ubt_isoc_out_complete */

Static void
ubt_isoc_out_complete2(node_p node, hook_p hook, void *arg1, int arg2)
{
	ubt_softc_p		sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	usbd_xfer_handle	h = (usbd_xfer_handle) arg1;
	usbd_status		s = (usbd_status) arg2;

	if (sc == NULL)
		return;

	KASSERT((sc->sc_flags & UBT_SCO_XMIT), (
"%s: %s - No isoc-out request is pending\n", __func__, USBDEVNAME(sc->sc_dev)));

	sc->sc_flags &= ~UBT_SCO_XMIT;

	if (s == USBD_CANCELLED) {
		NG_UBT_INFO(
"%s: %s - Isoc-out xfer cancelled, pipe=%p\n",
			__func__, USBDEVNAME(sc->sc_dev),
			sc->sc_isoc_out_pipe);

		return;
	}

	if (s != USBD_NORMAL_COMPLETION) {    
		NG_UBT_WARN(
"%s: %s - Isoc-out xfer failed. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(s), s);

		if (s == USBD_STALLED)
			usbd_clear_endpoint_stall_async(sc->sc_isoc_out_pipe);

		NG_UBT_STAT_OERROR(sc->sc_stat);
	} else {
		NG_UBT_INFO(
"%s: %s - Sent %d bytes to isoc-out pipe\n",
			__func__, USBDEVNAME(sc->sc_dev), h->actlen);

		NG_UBT_STAT_BYTES_SENT(sc->sc_stat, h->actlen);
		NG_UBT_STAT_PCKTS_SENT(sc->sc_stat);
	}

	if (NG_BT_MBUFQ_LEN(&sc->sc_scoq) > 0)
		ubt_isoc_out_start(sc);
} /* ubt_isoc_out_complete2 */

/*
 * Abort transfers on all USB pipes
 */

Static void
ubt_reset(ubt_softc_p sc)
{
	/* Interrupt */
	if (sc->sc_intr_pipe != NULL)
		usbd_abort_pipe(sc->sc_intr_pipe);

	/* Bulk-in/out */
	if (sc->sc_bulk_in_pipe != NULL)
		usbd_abort_pipe(sc->sc_bulk_in_pipe);
	if (sc->sc_bulk_out_pipe != NULL)
		usbd_abort_pipe(sc->sc_bulk_out_pipe);

	/* Isoc-in/out */
	if (sc->sc_isoc_in_pipe != NULL)
		usbd_abort_pipe(sc->sc_isoc_in_pipe);
	if (sc->sc_isoc_out_pipe != NULL)
		usbd_abort_pipe(sc->sc_isoc_out_pipe);

	/* Cleanup queues */
	NG_BT_MBUFQ_DRAIN(&sc->sc_cmdq);
	NG_BT_MBUFQ_DRAIN(&sc->sc_aclq);
	NG_BT_MBUFQ_DRAIN(&sc->sc_scoq);
} /* ubt_reset */

/****************************************************************************
 ****************************************************************************
 **                        Netgraph specific
 ****************************************************************************
 ****************************************************************************/

/*
 * Netgraph node constructor. Do not allow to create node of this type.
 */

Static int
ng_ubt_constructor(node_p node)
{
	return (EINVAL);
} /* ng_ubt_constructor */

/*
 * Netgraph node destructor. Destroy node only when device has been detached
 */

Static int
ng_ubt_shutdown(node_p node)
{
	ubt_softc_p	sc = (ubt_softc_p) NG_NODE_PRIVATE(node);

	/* Let old node go */
	NG_NODE_SET_PRIVATE(node, NULL);
	NG_NODE_UNREF(node);
	
	if (sc == NULL)
		goto done;

	/* Create Netgraph node */
	if (ng_make_node_common(&typestruct, &sc->sc_node) != 0) {
		printf("%s: Could not create Netgraph node\n",
			USBDEVNAME(sc->sc_dev));
		sc->sc_node = NULL;
		goto done;
	}
	
	/* Name node */	
	if (ng_name_node(sc->sc_node, USBDEVNAME(sc->sc_dev)) != 0) {
		printf("%s: Could not name Netgraph node\n",
			USBDEVNAME(sc->sc_dev));
		NG_NODE_UNREF(sc->sc_node);
		sc->sc_node = NULL;
		goto done;
	}

	NG_NODE_SET_PRIVATE(sc->sc_node, sc);
	NG_NODE_FORCE_WRITER(sc->sc_node);
done:
	return (0);
} /* ng_ubt_shutdown */

/*
 * Create new hook. There can only be one.
 */

Static int
ng_ubt_newhook(node_p node, hook_p hook, char const *name)
{
	ubt_softc_p	sc = (ubt_softc_p) NG_NODE_PRIVATE(node);

	/* Refuse to create new hook if device interface is active */
	if (sc->sc_ctrl_dev != NODEV || sc->sc_intr_dev != NODEV ||
	    sc->sc_bulk_dev != NODEV)
		return (EBUSY);

	if (strcmp(name, NG_UBT_HOOK) != 0)
		return (EINVAL);

	if (sc->sc_hook != NULL)
		return (EISCONN);

	sc->sc_hook = hook;

	return (0);
} /* ng_ubt_newhook */

/*
 * Connect hook. Start incoming USB transfers
 */

Static int
ng_ubt_connect(hook_p hook)
{
	ubt_softc_p	sc = (ubt_softc_p) NG_NODE_PRIVATE(NG_HOOK_NODE(hook));
	usbd_status	status;

	/* Refuse to connect hook if device interface is active */
	if (sc->sc_ctrl_dev != NODEV || sc->sc_intr_dev != NODEV ||
	    sc->sc_bulk_dev != NODEV)
		return (EBUSY);

	NG_HOOK_FORCE_QUEUE(NG_HOOK_PEER(hook));

	/* Start intr transfer */
	status = ubt_intr_start(sc);
	if (status != USBD_NORMAL_COMPLETION) {
		NG_UBT_ALERT(
"%s: %s - Could not start interrupt transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);
		goto fail;
	}

	/* Start bulk-in transfer */
	status = ubt_bulk_in_start(sc);
	if (status != USBD_NORMAL_COMPLETION) {
		NG_UBT_ALERT(
"%s: %s - Could not start bulk-in transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);
		goto fail;
	}

#if __broken__ /* XXX FIXME */
	/* Start isoc-in transfer */
	status = ubt_isoc_in_start(sc);
	if (status != USBD_NORMAL_COMPLETION) {
		NG_UBT_ALERT(
"%s: %s - Could not start isoc-in transfer. %s (%d)\n",
			__func__, USBDEVNAME(sc->sc_dev), usbd_errstr(status),
			status);
		goto fail;
	}
#endif /* __broken__ */

	return (0);
fail:
	ubt_reset(sc);
	sc->sc_hook = NULL;

	return (ENXIO);
} /* ng_ubt_connect */

/*
 * Disconnect hook
 */

Static int
ng_ubt_disconnect(hook_p hook)
{
	ubt_softc_p	sc = (ubt_softc_p) NG_NODE_PRIVATE(NG_HOOK_NODE(hook));

	if (sc != NULL) {
		if (hook != sc->sc_hook)
			return (EINVAL);

		ubt_reset(sc);
		sc->sc_hook = NULL;
	}

	return (0);
} /* ng_ubt_disconnect */

/*
 * Process control message
 */

Static int
ng_ubt_rcvmsg(node_p node, item_p item, hook_p lasthook)
{
	ubt_softc_p		 sc = (ubt_softc_p) NG_NODE_PRIVATE(node);
	struct ng_mesg		*msg = NULL, *rsp = NULL;
	struct ng_bt_mbufq	*q = NULL;
	int			 error = 0, queue, qlen;

	if (sc == NULL) {
		NG_FREE_ITEM(item);
		return (EHOSTDOWN);
	}

	NGI_GET_MSG(item, msg);

	switch (msg->header.typecookie) {
	case NGM_GENERIC_COOKIE:
		switch (msg->header.cmd) {
		case NGM_TEXT_STATUS:
			NG_MKRESPONSE(rsp, msg, NG_TEXTRESPONSE, M_NOWAIT);
			if (rsp == NULL)
				error = ENOMEM;
			else
				snprintf(rsp->data, NG_TEXTRESPONSE,
					"Hook: %s\n"   \
					"Flags: %#x\n" \
					"Debug: %d\n"  \
					"CMD queue: [have:%d,max:%d]\n" \
					"ACL queue: [have:%d,max:%d]\n" \
					"SCO queue: [have:%d,max:%d]",
					(sc->sc_hook != NULL)? NG_UBT_HOOK : "",
					sc->sc_flags,
					sc->sc_debug,
					NG_BT_MBUFQ_LEN(&sc->sc_cmdq),
					sc->sc_cmdq.maxlen,
					NG_BT_MBUFQ_LEN(&sc->sc_aclq),
					sc->sc_aclq.maxlen,
					NG_BT_MBUFQ_LEN(&sc->sc_scoq),
					sc->sc_scoq.maxlen);
			break;

		default:
			error = EINVAL;
			break;
		}
		break;

	case NGM_UBT_COOKIE:
		switch (msg->header.cmd) {
		case NGM_UBT_NODE_SET_DEBUG:
			if (msg->header.arglen != sizeof(ng_ubt_node_debug_ep))
				error = EMSGSIZE;
			else
				sc->sc_debug =
					*((ng_ubt_node_debug_ep *)(msg->data));
			break;

		case NGM_UBT_NODE_GET_DEBUG:
			NG_MKRESPONSE(rsp, msg, sizeof(ng_ubt_node_debug_ep),
				M_NOWAIT);
			if (rsp == NULL)
				error = ENOMEM;
			else
				*((ng_ubt_node_debug_ep *)(rsp->data)) = 
					sc->sc_debug;
                        break;

		case NGM_UBT_NODE_SET_QLEN:
			if (msg->header.arglen != sizeof(ng_ubt_node_qlen_ep))
				error = EMSGSIZE;
			else {
				queue = ((ng_ubt_node_qlen_ep *)
						(msg->data))->queue;
				qlen = ((ng_ubt_node_qlen_ep *)
						(msg->data))->qlen;

				if (qlen <= 0) {
					error = EINVAL;
					break;
				}

				switch (queue) {
				case NGM_UBT_NODE_QUEUE_CMD:
					q = &sc->sc_cmdq;
					break;

				case NGM_UBT_NODE_QUEUE_ACL:
					q = &sc->sc_aclq;
					break;

				case NGM_UBT_NODE_QUEUE_SCO:
					q = &sc->sc_scoq;
					break;

				default:
					q = NULL;
					error = EINVAL;
					break;
				}

				if (q != NULL)
					q->maxlen = qlen;
			}
			break;

		case NGM_UBT_NODE_GET_QLEN:
			if (msg->header.arglen != sizeof(ng_ubt_node_qlen_ep)) {
				error = EMSGSIZE;
				break;
			}

			queue = ((ng_ubt_node_qlen_ep *)(msg->data))->queue;
			switch (queue) {
			case NGM_UBT_NODE_QUEUE_CMD:
				q = &sc->sc_cmdq;
				break;

			case NGM_UBT_NODE_QUEUE_ACL:
				q = &sc->sc_aclq;
				break;

			case NGM_UBT_NODE_QUEUE_SCO:
				q = &sc->sc_scoq;
				break;

			default:
				q = NULL;
				error = EINVAL;
				break;
			}

			if (q != NULL) {
				NG_MKRESPONSE(rsp, msg, 
					sizeof(ng_ubt_node_qlen_ep), M_NOWAIT);
				if (rsp == NULL) {
					error = ENOMEM;
					break;
				}

				((ng_ubt_node_qlen_ep *)(rsp->data))->queue =
					queue;
				((ng_ubt_node_qlen_ep *)(rsp->data))->qlen =
					q->maxlen;
			}
			break;

		case NGM_UBT_NODE_GET_STAT:
			NG_MKRESPONSE(rsp, msg, sizeof(ng_ubt_node_stat_ep),
				M_NOWAIT);
			if (rsp == NULL)
				error = ENOMEM;
			else
				bcopy(&sc->sc_stat, rsp->data,
					sizeof(ng_ubt_node_stat_ep));
			break;

		case NGM_UBT_NODE_RESET_STAT:
			NG_UBT_STAT_RESET(sc->sc_stat);
			break;

		case NGM_UBT_NODE_DEV_NODES:
			if (msg->header.arglen !=
					sizeof(ng_ubt_node_dev_nodes_ep)) {
				error = EMSGSIZE;
				break;
			}

			if ((sc->sc_flags & UBT_ANY_DEV) || 
			    sc->sc_hook != NULL) {
				error = EBUSY;
				break;
			}

			if (*((ng_ubt_node_dev_nodes_ep *)(msg->data)))
				ubt_create_device_nodes(sc);
			else
				ubt_destroy_device_nodes(sc);
			break;

		default:
			error = EINVAL;
			break;
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	NG_RESPOND_MSG(error, node, item, rsp);
	NG_FREE_MSG(msg);

	return (error);
} /* ng_ubt_rcvmsg */

/*
 * Process data
 */

Static int
ng_ubt_rcvdata(hook_p hook, item_p item)
{
	ubt_softc_p		 sc = (ubt_softc_p) NG_NODE_PRIVATE(NG_HOOK_NODE(hook));
	struct mbuf		*m = NULL;
	usbd_status		(*f)(ubt_softc_p) = NULL;
	struct ng_bt_mbufq	*q = NULL;
	int			 b, error = 0;

	if (sc == NULL) {
		error = EHOSTDOWN;
		goto done;
	}

	if (hook != sc->sc_hook) {
		error = EINVAL;
		goto done;
	}

	/* Deatch mbuf and get HCI frame type */
	NGI_GET_M(item, m);

	/* Process HCI frame */
	switch (*mtod(m, u_int8_t *)) { /* XXX call m_pullup ? */
	case NG_HCI_CMD_PKT:
		f = ubt_request_start;
		q = &sc->sc_cmdq;
		b = UBT_CMD_XMIT;
		break;

	case NG_HCI_ACL_DATA_PKT:
		f = ubt_bulk_out_start;
		q = &sc->sc_aclq;
		b = UBT_ACL_XMIT;
		break;

#if __broken__ /* XXX FIXME */
	case NG_HCI_SCO_DATA_PKT:
		f = ubt_isoc_out_start;
		q = &sc->sc_scoq;
		b = UBT_SCO_XMIT;
		break;
#endif /* __broken__ */

	default:
		NG_UBT_ERR(
"%s: %s - Dropping unknown/unsupported HCI frame, type=%d, pktlen=%d\n",
			__func__, USBDEVNAME(sc->sc_dev), *mtod(m, u_int8_t *),
			m->m_pkthdr.len);

		NG_FREE_M(m);
		error = EINVAL;

		goto done;
		/* NOT REACHED */
	}

	/* Loose frame type, if required */
	if (!(sc->sc_flags & UBT_NEED_FRAME_TYPE))
		m_adj(m, sizeof(u_int8_t)); 

	if (NG_BT_MBUFQ_FULL(q)) {
		NG_UBT_ERR(
"%s: %s - Dropping HCI frame %#x, len=%d. Queue full\n",
			__func__, USBDEVNAME(sc->sc_dev),
			*mtod(m, u_int8_t *), m->m_pkthdr.len);

		NG_FREE_M(m);
	} else
		NG_BT_MBUFQ_ENQUEUE(q, m);

	if (!(sc->sc_flags & b))
		if ((*f)(sc) != USBD_NORMAL_COMPLETION)
			error = EIO;
done:
	NG_FREE_ITEM(item);

	return (error);
} /* ng_ubt_rcvdata */

/****************************************************************************
 ****************************************************************************
 **                              Device specific
 ****************************************************************************
 ****************************************************************************/

/*
 * Open endpoint device
 * XXX FIXME softc locking
 */

Static int
ubt_open(dev_t dev, int flag, int mode, usb_proc_ptr p)
{
	ubt_softc_p	sc = NULL;
	int		ep = UBT_ENDPOINT(dev);

	USB_GET_SC_OPEN(ubt, UBT_UNIT(dev), sc); /* check for sc != NULL */
	if (sc->sc_dying)
		return (ENXIO);

	if (ep == USB_CONTROL_ENDPOINT) {
		if (sc->sc_flags & UBT_CTRL_DEV)
			return (EBUSY);

		sc->sc_flags |= UBT_CTRL_DEV;
	} else if (ep == UE_GET_ADDR(sc->sc_intr_ep)) {
		if (sc->sc_flags & UBT_INTR_DEV)
			return (EBUSY);
		if (sc->sc_intr_pipe == NULL)
			return (ENXIO);

		sc->sc_flags |= UBT_INTR_DEV;
	} else if (ep == UE_GET_ADDR(sc->sc_bulk_in_ep)) {
		if (sc->sc_flags & UBT_BULK_DEV)
			return (EBUSY);
		if (sc->sc_bulk_in_pipe == NULL || sc->sc_bulk_out_pipe == NULL)
			return (ENXIO);

		sc->sc_flags |= UBT_BULK_DEV;
	} else
		return (EINVAL);

	return (0);
} /* ubt_open */

/*
 * Close endpoint device
 * XXX FIXME softc locking
 */

Static int
ubt_close(dev_t dev, int flag, int mode, usb_proc_ptr p)
{
	ubt_softc_p	sc = NULL;
	int		ep = UBT_ENDPOINT(dev);

	USB_GET_SC(ubt, UBT_UNIT(dev), sc);
	if (sc == NULL)
		return (ENXIO);

	if (ep == USB_CONTROL_ENDPOINT)
		sc->sc_flags &= ~UBT_CTRL_DEV;
	else if (ep == UE_GET_ADDR(sc->sc_intr_ep)) {
		if (sc->sc_intr_pipe != NULL)
			usbd_abort_pipe(sc->sc_intr_pipe);

		sc->sc_flags &= ~UBT_INTR_DEV;
	} else if (ep == UE_GET_ADDR(sc->sc_bulk_in_ep)) {
		/* Close both in and out bulk pipes */
		if (sc->sc_bulk_in_pipe != NULL)
			usbd_abort_pipe(sc->sc_bulk_in_pipe);

		if (sc->sc_bulk_out_pipe != NULL)
			usbd_abort_pipe(sc->sc_bulk_out_pipe);

		sc->sc_flags &= ~UBT_BULK_DEV;
	} else
		return (EINVAL);

	return (0);
} /* ubt_close */

/*
 * Read from the endpoint device
 * XXX FIXME softc locking
 */

Static int
ubt_read(dev_t dev, struct uio *uio, int flag)
{
	ubt_softc_p		sc = NULL;
	int			error = 0, n, tn, ep = UBT_ENDPOINT(dev);
	usbd_status		status;
	usbd_pipe_handle	pipe = NULL;
	usbd_xfer_handle	xfer = NULL;
	u_int8_t		buf[UBT_BSIZE];

	USB_GET_SC(ubt, UBT_UNIT(dev), sc);
	if (sc == NULL || sc->sc_dying)
		return (ENXIO);

	if (ep == USB_CONTROL_ENDPOINT)
		return (EOPNOTSUPP);

	if (ep == UE_GET_ADDR(sc->sc_intr_ep)) {
		pipe = sc->sc_intr_pipe;
		xfer = sc->sc_intr_xfer;
	} else if (ep == UE_GET_ADDR(sc->sc_bulk_in_ep)) {
		pipe = sc->sc_bulk_in_pipe;
		xfer = sc->sc_bulk_in_xfer;
	} else
		return (EINVAL);

	if (pipe == NULL || xfer == NULL)
		return (ENXIO);

	sc->sc_refcnt ++;

	while ((n = min(sizeof(buf), uio->uio_resid)) != 0) {
		tn = n;
		status = usbd_bulk_transfer(xfer, pipe, USBD_SHORT_XFER_OK,
				USBD_DEFAULT_TIMEOUT, buf, &tn, "ubtrd");
		switch (status) {
		case USBD_NORMAL_COMPLETION:
			error = uiomove(buf, tn, uio);
			break;

		case USBD_INTERRUPTED:
			error = EINTR;
			break;

		case USBD_TIMEOUT:
			error = ETIMEDOUT;
			break;

		default:
			error = EIO;
			break;
		}

		if (error != 0 || tn < n)
			break;
	}

	if (-- sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	return (error);
} /* ubt_read */

/*
 * Write into the endpoint device
 * XXX FIXME softc locking
 */

Static int
ubt_write(dev_t dev, struct uio *uio, int flag)
{
	ubt_softc_p	sc = NULL;
	int		error = 0, n, ep = UBT_ENDPOINT(dev);
	usbd_status	status;
	u_int8_t	buf[UBT_BSIZE];

	USB_GET_SC(ubt, UBT_UNIT(dev), sc);
	if (sc == NULL || sc->sc_dying)
		return (ENXIO);

	if (ep == USB_CONTROL_ENDPOINT || ep == UE_GET_ADDR(sc->sc_intr_ep))
		return (EOPNOTSUPP);
	if (ep != UE_GET_ADDR(sc->sc_bulk_in_ep))
		return (EINVAL);

	sc->sc_refcnt ++;

	while ((n = min(sizeof(buf), uio->uio_resid)) != 0) {
		error = uiomove(buf, n, uio);
		if (error != 0)
			break;

		status = usbd_bulk_transfer(sc->sc_bulk_out_xfer,
				sc->sc_bulk_out_pipe, 0, USBD_DEFAULT_TIMEOUT,
				buf, &n,"ubtwr");
		switch (status) {
		case USBD_NORMAL_COMPLETION:
			break;

		case USBD_INTERRUPTED:
			error = EINTR;
			break;

		case USBD_TIMEOUT:
			error = ETIMEDOUT;
			break;

		default:
			error = EIO;
			break;
		}

		if (error != 0)
			break;
	}

	if (-- sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	return (error);
} /* ubt_write */

/*
 * Process ioctl on the endpoint device. Mostly stolen from ugen(4)
 * XXX FIXME softc locking
 */

Static int
ubt_ioctl(dev_t dev, u_long cmd, caddr_t data, int flag, usb_proc_ptr p)
{
	ubt_softc_p		 sc = NULL;
	int			 len, error = 0, ep = UBT_ENDPOINT(dev);
	usbd_status		 status;
	struct usb_string_desc	*si = NULL;
	struct usb_ctl_request	*ur = NULL;
	void			*ptr = NULL;
	struct iovec		 iov;
	struct uio		 uio;

	USB_GET_SC(ubt, UBT_UNIT(dev), sc);
	if (sc == NULL || sc->sc_dying)
		return (ENXIO);

	if (ep != USB_CONTROL_ENDPOINT)
		return (EOPNOTSUPP);

	sc->sc_refcnt ++;

	switch (cmd) {
	case USB_GET_DEVICE_DESC:
		*(usb_device_descriptor_t *) data =
				*usbd_get_device_descriptor(sc->sc_udev);
		break;

	case USB_GET_STRING_DESC:
		si = (struct usb_string_desc *) data;
		status = usbd_get_string_desc(sc->sc_udev, si->usd_string_index,
				si->usd_language_id, &si->usd_desc);
		if (status != USBD_NORMAL_COMPLETION)
			error = EINVAL;
		break;

	case USB_DO_REQUEST:
		ur = (void *) data;
		len = UGETW(ur->ucr_request.wLength);

		if (!(flag & FWRITE)) {
			error = EPERM;
			break;
		}

		/* Avoid requests that would damage the bus integrity. */
		if ((ur->ucr_request.bmRequestType == UT_WRITE_DEVICE &&
		     ur->ucr_request.bRequest == UR_SET_ADDRESS) ||
		    (ur->ucr_request.bmRequestType == UT_WRITE_DEVICE &&
		     ur->ucr_request.bRequest == UR_SET_CONFIG) ||
		    (ur->ucr_request.bmRequestType == UT_WRITE_INTERFACE &&
		     ur->ucr_request.bRequest == UR_SET_INTERFACE) ||
		    len < 0 || len > 32767) {
			error = EINVAL;
			break;
		}

		if (len != 0) {
			iov.iov_base = (caddr_t) ur->ucr_data;
			iov.iov_len = len;

			uio.uio_iov = &iov;
			uio.uio_iovcnt = 1;
			uio.uio_resid = len;
			uio.uio_offset = 0;
			uio.uio_segflg = UIO_USERSPACE;
			uio.uio_rw = ur->ucr_request.bmRequestType & UT_READ ?
						UIO_READ : UIO_WRITE;
			uio.uio_procp = p;

			ptr = malloc(len, M_TEMP, M_WAITOK);
			if (uio.uio_rw == UIO_WRITE) {
				error = uiomove(ptr, len, &uio);
				if (error != 0)
					goto ret;
			}
		}

		status = usbd_do_request_flags(sc->sc_udev, &ur->ucr_request,
				ptr, ur->ucr_flags, &ur->ucr_actlen,
				USBD_DEFAULT_TIMEOUT);
		if (status != USBD_NORMAL_COMPLETION) {
			error = EIO;
			goto ret;
		}

		if (len != 0) {
			if (uio.uio_rw == UIO_READ) {
				error = uiomove(ptr, len, &uio);
				if (error != 0)
					goto ret;
			}
		}
ret:
		if (ptr != NULL)
			free(ptr, M_TEMP);
		break;

	case USB_GET_DEVICEINFO:
		usbd_fill_deviceinfo(sc->sc_udev,
			(struct usb_device_info *) data, 1);
		break;

	default:
		error = EINVAL;
		break;
	}

	if (-- sc->sc_refcnt < 0)
		usb_detach_wakeup(USBDEV(sc->sc_dev));

	return (error);
} /* ubt_ioctl */

/*
 * Poll the endpoint device
 * XXX FIXME softc locking
 */

Static int
ubt_poll(dev_t dev, int events, usb_proc_ptr p)
{
	ubt_softc_p	sc = NULL;
	int		revents = 0, ep = UBT_ENDPOINT(dev);

	USB_GET_SC(ubt, UBT_UNIT(dev), sc);
	if (sc == NULL || sc->sc_dying)
		return (ENXIO);

	if (ep == USB_CONTROL_ENDPOINT)
		return (EOPNOTSUPP);

	if (ep == UE_GET_ADDR(sc->sc_intr_ep)) {
		if (sc->sc_intr_pipe != NULL)
			revents |= events & (POLLIN | POLLRDNORM);
		else
			revents = EIO;
	} else if (ep == UE_GET_ADDR(sc->sc_bulk_in_ep)) {
		if (sc->sc_bulk_in_pipe != NULL) 
			revents |= events & (POLLIN | POLLRDNORM);

		if (sc->sc_bulk_out_pipe != NULL)
			revents |= events & (POLLOUT | POLLWRNORM);

		if (revents == 0)
			revents = EIO; /* both pipes closed */
	} else
		revents = EINVAL;

	return (revents);
} /* ubt_poll */

/*
 * Create device nodes for all endpoints. Must be called with node locked.
 */

Static void
ubt_create_device_nodes(ubt_softc_p sc)
{
	int	ep;

	KASSERT((sc->sc_hook == NULL), (
"%s: %s - hook != NULL!\n", __func__, USBDEVNAME(sc->sc_dev)));

	/* Control device */
	if (sc->sc_ctrl_dev == NODEV)
		sc->sc_ctrl_dev = make_dev(&ubt_cdevsw,
					UBT_MINOR(USBDEVUNIT(sc->sc_dev), 0),
					UID_ROOT, GID_OPERATOR, 0644,
					"%s", USBDEVNAME(sc->sc_dev));

	/* Interrupt device */
	if (sc->sc_intr_dev == NODEV && sc->sc_intr_ep != -1) {
		ep = UE_GET_ADDR(sc->sc_intr_ep);
		sc->sc_intr_dev = make_dev(&ubt_cdevsw,
					UBT_MINOR(USBDEVUNIT(sc->sc_dev), ep),
					UID_ROOT, GID_OPERATOR, 0644,
					"%s.%d", USBDEVNAME(sc->sc_dev), ep);
	}

	/* 
	 * Bulk-in and bulk-out device
	 * XXX will create one device for both in and out endpoints.
	 * XXX note that address of the in and out endpoint should be the same
	 */

	if (sc->sc_bulk_dev == NODEV && 
	    sc->sc_bulk_in_ep != -1 && sc->sc_bulk_out_ep != -1 &&
	    UE_GET_ADDR(sc->sc_bulk_in_ep) == UE_GET_ADDR(sc->sc_bulk_out_ep)) {
		ep = UE_GET_ADDR(sc->sc_bulk_in_ep);
		sc->sc_bulk_dev = make_dev(&ubt_cdevsw,
					UBT_MINOR(USBDEVUNIT(sc->sc_dev), ep),
					UID_ROOT, GID_OPERATOR, 0644,
					"%s.%d", USBDEVNAME(sc->sc_dev), ep);
	}
} /* ubt_create_device_nodes */

/*
 * Destroy device nodes for all endpoints
 * XXX FIXME locking
 */

Static void
ubt_destroy_device_nodes(ubt_softc_p sc)
{
	/*
	 * Wait for processes to go away. This should be safe as we will not
	 * call ubt_destroy_device_nodes() from Netgraph unless all devices
	 * were closed (and thus no active processes).
	 */

	if (-- sc->sc_refcnt >= 0) {
		ubt_reset(sc);
		usb_detach_wait(USBDEV(sc->sc_dev));
	}

	sc->sc_refcnt = 0;

	/* Destroy device nodes */
	if (sc->sc_bulk_dev != NODEV) {
		destroy_dev(sc->sc_bulk_dev);
		sc->sc_bulk_dev = NODEV;
	}

	if (sc->sc_intr_dev != NODEV) {
		destroy_dev(sc->sc_intr_dev);
		sc->sc_intr_dev = NODEV;
	}

	if (sc->sc_ctrl_dev != NODEV) {
		destroy_dev(sc->sc_ctrl_dev);
		sc->sc_ctrl_dev = NODEV;
	}
} /* ubt_destroy_device_nodes */

