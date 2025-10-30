#ifndef _HGIC_PORTING_USB_H_
#define _HGIC_PORTING_USB_H_

#define URB_ZERO_PACKET		    0x0040	    /* Finish bulk OUT with short packet */

typedef void usb_device_t;
typedef void usb_interface_t;
typedef void urb_t;
typedef void usb_device_id_t;
typedef void (*usb_complete_cb)(urb_t *);

#define usb_alloc_urb       sys_usb_alloc_urb
#define usb_free_urb        sys_usb_free_urb
#define usb_fill_bulk_urb   sys_usb_fill_bulk_urb
#define usb_submit_urb      sys_usb_submit_urb
#define usb_sndbulkpipe     sys_usb_sndbulkpipe
#define usb_rcvbulkpipe    sys_usb_rcvbulkpipe
#define usb_kill_urb        sys_usb_kill_urb
#define usb_unlink_urb      sys_usb_unlink_urb
#define usb_get_urb_ref     sys_usb_get_urb_ref

#define interface_to_usbdev sys_interface_to_usbdev
#define usbdev_to_dev       sys_usbdev_to_dev
#define usb_set_intfdata    sys_usb_set_intfdata
#define usb_get_intfdata    sys_usb_get_intfdata
#define usb_get_dev         sys_usb_get_dev
#define up(sem)             os_sema_up((sem))

extern void hgic_usb_init_endpoint(void *udev, usb_interface_t *iface);
extern void *urb_get_context(urb_t *urb);
extern int   urb_get_actual_length(urb_t *urb);
extern int   urb_get_status(urb_t *urb);
extern void  urb_set_xfer_flags(urb_t *urb,unsigned int flags);
extern unsigned short usb_get_vendor(usb_device_id_t *id);
extern unsigned short usb_get_product(usb_device_id_t *id);
extern void sys_cache_flush(unsigned int addr,unsigned int len);
extern void sys_cache_invalidate(unsigned int addr,unsigned int len);
extern int  sys_usb_init(void);
extern void sys_usb_exit(void);

#endif

