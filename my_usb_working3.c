#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
 
#include <asm/uaccess.h>
#include<linux/slab.h>
#include <linux/usbdevice_fs.h>

#include "ioctl.h"

#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define MAX_PKT_SIZE 512
 
struct urb *urb;
static struct usb_device *device;
struct usb_endpoint_descriptor *endpoint;
static struct usb_class_driver class;
static char *trans_buf;
 
int bulk_in_endpointAddr;
unsigned int bulk_in_size;
char *bulk_in_buffer;
int bulk_out_endpointAddr;

static void usb_irq(void *data)
{
	switch (urb->status) {
        case 0:                 /* success */
		break;
        case -ECONNRESET:       /* unlink */
        case -ENOENT:
        case -ESHUTDOWN:
		printk("Error in callback status %d\n", urb->status);
                return;
         /* -EPIPE:  should clear the halt */
        }

/* free up our allocated buffer */
//	usb_free_coherent(device, 64, trans_buf, &urb->transfer_dma);
}

static int pen_open(struct inode *i, struct file *f)
{
    return 0;
}
static int pen_close(struct inode *i, struct file *f)
{

//usb_kill_urb(urb);	
   return 0;
}

static ssize_t pen_read(struct file *f, char __user *buf_t, size_t cnt, loff_t *off)
{
	int retval = 0;
	int read_cnt = MIN(cnt, MAX_PKT_SIZE);
 
	printk(KERN_ERR "Reading start \n");

	urb = usb_alloc_urb(0, GFP_KERNEL);
	trans_buf = usb_alloc_coherent(device, read_cnt, GFP_KERNEL, &urb->transfer_dma);
	usb_fill_int_urb(urb, device, usb_rcvintpipe(device, bulk_in_endpointAddr), trans_buf, read_cnt, usb_irq, trans_buf, 2);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	retval = usb_submit_urb(urb, GFP_ATOMIC);
    	if (retval)
    	{
        	printk(KERN_ERR "Error message returned %d\n", retval);
        	goto error;
   	}
 
	memcpy(buf_t, trans_buf, read_cnt);
        printk(KERN_ERR "Reading done\n");
error:
	return retval;
}

static ssize_t pen_write(struct file *f, const char __user *buf_t, size_t cnt, loff_t *off)
{
    int retval = 0;
    int write_cnt = MIN(cnt, MAX_PKT_SIZE);
 
	printk(KERN_ERR "Writting start  %s\n", buf_t);

	urb = usb_alloc_urb(0, GFP_KERNEL);
	trans_buf = usb_alloc_coherent(device, write_cnt, GFP_KERNEL, &urb->transfer_dma);
	memcpy(trans_buf, buf_t, write_cnt);
	printk("sending data %s\n", buf_t);
	usb_fill_int_urb(urb, device, usb_sndintpipe(device, bulk_out_endpointAddr), trans_buf, write_cnt, usb_irq, trans_buf, 2);
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	retval = usb_submit_urb(urb, GFP_ATOMIC);
    	if (retval)
    	{
        	printk(KERN_ERR "Error message returned %d\n", retval);
        	goto error;
   	 }
 
         printk(KERN_ERR "Writting done\n");
error:
	return retval;
}
 
static int device_ioctl(struct inode *inode,
			struct file *file,
    			unsigned int ioctl_num,/* The number of the ioctl */
   			unsigned long ioctl_param)
{

	char *buf = kzalloc(MSG_SIZE, GFP_KERNEL);
	loff_t offset = 0;
	int ret;
	struct dev_ioctl *dev_io;
	int cmd;

	printk("ioctl_num %u\n", ioctl_num);
	ret = get_user(cmd, (int  *)ioctl_param);
	//printk("char ioctl_param %c\n",dev_io->cmd);
	switch(cmd) {
	case USB_LOW:
		buf[0] = USB_LOW;
		ret = pen_write(file, buf, MSG_SIZE, &offset); 
		break;
	case USB_HIGH:
		buf[0] = USB_HIGH;
		ret = pen_write(file, buf, MSG_SIZE, &offset); 
		break;
	case USB_READ:
		ret = pen_read(file, buf , MSG_SIZE, &offset);
		//copy_to_user((void *)ioctl_param, buf, MSG_SIZE);

		printk("read data %s\n", buf);
		break;
	default:
		ret = -1;
		goto error;
	}

error:
	return ret;
}

int device_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static struct file_operations fops =
{
    .open = pen_open,
    .release = pen_close,
    .read = pen_read,
    .write = pen_write,
    .unlocked_ioctl = device_ioctl,
};
 
static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    int retval = 0, i;
    struct usb_host_interface *iface_desc;
	unsigned int buffer_size;

	device = interface_to_usbdev(interface);
    //endpoint = &interface->endpoint[0].desc;

    /* set up the endpoint information */
    /* use only the first bulk-in and bulk-out endpoints */
    iface_desc = interface->cur_altsetting;
    for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
	    endpoint = &iface_desc->endpoint[i].desc;
	    if (!bulk_in_endpointAddr &&
			    (endpoint->bEndpointAddress & USB_DIR_IN) &&
			    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
			     == USB_ENDPOINT_XFER_INT)) {
		    /* we found a bulk in endpoint */
		    buffer_size = endpoint->wMaxPacketSize;
		    bulk_in_size = buffer_size;
		    bulk_in_endpointAddr = endpoint->bEndpointAddress;
		    bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
		    if (!bulk_in_buffer) {
			    printk("Could not allocate bulk_in_buffer");
			    goto error;
		    }
	    }
	    if (!bulk_out_endpointAddr &&
			    !(endpoint->bEndpointAddress & USB_DIR_IN) &&
			    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
			     == USB_ENDPOINT_XFER_INT)) {
		    /* we found a bulk out endpoint */
		    bulk_out_endpointAddr = endpoint->bEndpointAddress;
	    }
    }
    if (!(bulk_in_endpointAddr && bulk_out_endpointAddr)) {
	    printk("Could not find both bulk-in and bulk-out endpoints");
	    goto error;
    }

	printk("bulk_in_endpointAddr %d  bulk_out_endpointAddr %d \n", bulk_in_endpointAddr, bulk_out_endpointAddr);

    class.name = "usb/pen%d";
    class.fops = &fops;
    if ((retval = usb_register_dev(interface, &class)) < 0)
    {
        /* Something prevented us from registering this driver */
        printk(KERN_ERR "Not able to get a minor for this device.");
    }
    else
    {
        printk(KERN_INFO "Minor obtained: %d\n", interface->minor);
    }
 
    printk(KERN_ERR "New USB device detected\n");

error:
    return retval;
}
 
static void pen_disconnect(struct usb_interface *interface)
{
	usb_kill_urb(urb);
	usb_deregister_dev(interface, &class);
	usb_free_urb(urb);
 	usb_free_coherent(device, MSG_SIZE, trans_buf, urb->transfer_dma);	

	printk(KERN_ERR "usb disconnected\n");
}
 
/* Table of devices that work with this driver */
static struct usb_device_id pen_table[] =
{
    //{ USB_DEVICE(0x0781, 0x557c) },
    { USB_DEVICE(0x16c0, 0x0480) },
    {} /* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, pen_table);
 
static struct usb_driver pen_driver =
{
    .name = "pen_driver",
    .probe = pen_probe,
    .disconnect = pen_disconnect,
    .id_table = pen_table,
};
 
static int __init pen_init(void)
{
    int result;
 
    /* Register this driver with the USB subsystem */
    if ((result = usb_register(&pen_driver)))
    {
        printk(KERN_ERR "usb_register failed. Error number %d", result);
    }
   printk(KERN_ERR "my usb register succesfull\n");
    return result;
}
 
static void __exit pen_exit(void)
{
    /* Deregister this driver with the USB subsystem */
    usb_deregister(&pen_driver);
    printk(KERN_ERR "unloading my usb driver\n");
}
 
module_init(pen_init);
module_exit(pen_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Surendra Mobiya");
MODULE_DESCRIPTION("USB Pen Device Driver");
