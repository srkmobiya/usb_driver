#include <linux/ioctl.h>
#define MAJOR_NUM 100
#define MSG_SIZE 64
#define USB_LOW 'l'
#define USB_HIGH 'h'
#define USB_READ 'r'
#define LED_ON  _IOW(MAJOR_NUM, USB_HIGH, void *)
#define LED_OFF _IOW(MAJOR_NUM, USB_LOW, void *)
#define USB_READ_IO _IOR(MAJOR_NUM, USB_READ, void *)

struct dev_ioctl {
        int cmd;
        char data[MSG_SIZE];
};

