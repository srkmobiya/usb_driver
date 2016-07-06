#include<stdio.h>
#include "ioctl.h"
#include<string.h>
#include <linux/usbdevice_fs.h>

static int usbdev_ioctl (int fd, int ifno, unsigned request, void *param)
{
        struct usbdevfs_ioctl	wrapper;
	struct usbdevfs_ioctl *w = &wrapper;

	int ret;

        wrapper.ifno = ifno;
        wrapper.ioctl_code = request;
        wrapper.data = param;

        ret = ioctl (fd, USBDEVFS_IOCTL, &wrapper);
	return ret;
} 

int main()
{
	int ret, file, cmd;
	char *msg = malloc(MSG_SIZE);

	file = open("/dev/pen0", 0);
	if (file < 0) {
		printf("error in open file\n");
		exit(-1);
	}
	printf("Enter command:\n1. LED OFF\n2. LED ON\n3. Read USB Interrup\ninput: ");
	scanf("%d", &cmd);

	if (cmd == 1) {
		ret = usbdev_ioctl(file, 0, USB_LOW, msg);
	}
	else if (cmd == 2) {
		ret = usbdev_ioctl(file, 0, USB_HIGH, msg);
	}
	else if (cmd == 3) {
		ret = usbdev_ioctl(file, 0, USB_READ, msg);
		printf("Data read: \"%s\"\n", msg);
	}
	else
		printf("Wrong command\n");
		
	if (!ret)
		printf("Successfull\n");
	else
		printf("Error: ret %d\n", ret);

}
