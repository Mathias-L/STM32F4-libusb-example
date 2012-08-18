/*
 * Simple libusb-1.0 benchmark programm
 * It openes an USB device, expects two Bulk endpoints,
 *   EP1 should be IN
 *   EP2 should be OUT
 * It uses Synchronous device I/O
 *
 * Compile:
 *   gcc -lusb-1.0 -lrt -o bench benchmark.c
 * Run:
 *   ./bench
 * As far as I know, sys/time.h is POSIX only and not available on Windows.
 * It should be trivial to exchange for a precise Windows time API.
 * For Documentation on libusb see:
 *   http://libusb.sourceforge.net/api-1.0/modules.html
 */
#include <stdio.h>
#include <stdlib.h>

#include <signal.h>

//change if your libusb.h is located elswhere
#include <libusb-1.0/libusb.h>
//or uncomment this line:
//#include <libusb.h>
//and compile with:
//gcc -lusb-1.0 -lrt -o bench -I/path/to/libusb-1.0/ benchmark.c

#include <sys/time.h>

#define USB_VENDOR_ID	    0x0483      /* USB vendor ID used by the device
                                         * 0x0483 is STMs ID
                                         */
#define USB_PRODUCT_ID	    0xFFFF      /* USB product ID used by the device */
#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 1)   /* endpoint address */
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2)   /* endpoint address */
#define USB_TIMEOUT	        3000        /* Connection timeout (in ms) */

static libusb_context *ctx = NULL;
static libusb_device_handle *handle;

static uint8_t receiveBuf[1024*256*4];
uint8_t transferBuf[64];

uint16_t counter=0;

uint32_t benchPackets=1;
uint32_t benchBytes=0;
struct timespec t1, t2;
uint32_t diff=0;

/*
 * Read a packet
 */
 static int usb_read(void)
 {
 	int nread, ret;
 	//blocking synchronous call to libUSB when it finished, one packet was received
 	ret = libusb_bulk_transfer(handle, USB_ENDPOINT_IN, receiveBuf, sizeof(receiveBuf),
 		&nread, USB_TIMEOUT);
	benchBytes =nread;
	clock_gettime(CLOCK_REALTIME, &t2);

	//Warning: uint32_t has a max value of 4294967296 so this will overflow over 4secs
	diff = (t2.tv_sec-t1.tv_sec)*1000000000L+(t2.tv_nsec-t1.tv_nsec);
    t1.tv_sec = t2.tv_sec;
 	t1.tv_nsec = t2.tv_nsec;
 	printf("\rreceived %5d transfers and %8d bytes in %8d us, %8.1f B/s, transfersize: %d", benchPackets, benchBytes, diff/1000, benchBytes*1000000.0/(diff/1000), nread);
    fflush(stdout);
 }


/*
 * write a few bytes to the device
 * this function is unused for benchmarking
 */
 uint16_t count=0;
 static int usb_write(void)
 {
 	int n, ret;
    //count up
 	n = sprintf((char*)transferBuf, "%d\0",count++);
    //write transfer
 	ret = libusb_bulk_transfer(handle, USB_ENDPOINT_OUT, transferBuf, n,
 		&n, USB_TIMEOUT);
    //Error handling
 	switch(ret){
 		case 0:
 		printf("send %d bytes to device\n", n);
 		return 0;
 		case LIBUSB_ERROR_TIMEOUT:
 		printf("ERROR in bulk write: %d Timeout\n", ret);
 		break;
 		case LIBUSB_ERROR_PIPE:
 		printf("ERROR in bulk write: %d Pipe\n", ret);
 		break;
 		case LIBUSB_ERROR_OVERFLOW:
 		printf("ERROR in bulk write: %d Overflow\n", ret);
 		break;
 		case LIBUSB_ERROR_NO_DEVICE:
 		printf("ERROR in bulk write: %d No Device\n", ret);
 		break;
 		default:
 		printf("ERROR in bulk write: %d\n", ret);
 		break;

 	}
 	return -1;

 }

/*
 * on SIGINT: close USB interface
 * This still leads to a segfault on my system...
 */
 static void sighandler(int signum)
 {
 	printf( "\nInterrupt signal received\n" );
 	if (handle){
 		libusb_release_interface (handle, 0);
 		printf( "\nInterrupt signal received1\n" );
 		libusb_close(handle);
 		printf( "\nInterrupt signal received2\n" );
 	}
 	printf( "\nInterrupt signal received3\n" );
 	libusb_exit(NULL);
 	printf( "\nInterrupt signal received4\n" );

 	exit(0);
 }

 /*
  * main loop
  * inits the USB and starts the receiving loop
  */
 int main(int argc, char **argv)
 {
    //Pass Interrupt Signal to our handler
 	signal(SIGINT, sighandler);

    //init libUSB
 	libusb_init(&ctx);
 	libusb_set_debug(ctx, 3);

    //Open Device with VendorID and ProductID
 	handle = libusb_open_device_with_vid_pid(ctx,
 		USB_VENDOR_ID, USB_PRODUCT_ID);
 	if (!handle) {
 		perror("device not found");
 		return 1;
 	}

 	int r = 1;
	//Claim Interface 0 from the device
 	r = libusb_claim_interface(handle, 0);
 	if (r < 0) {
 		fprintf(stderr, "usb_claim_interface error %d\n", r);
 		return 2;
 	}
 	printf("Interface claimed\n");
 	//take the first time measurement
 	clock_gettime(CLOCK_REALTIME, &t1);

    //continuously read USB transfers
 	while (1){
 		usb_read();
//		usb_write();
 	}
    //never reached
 	libusb_close(handle);
 	libusb_exit(NULL);

 	return 0;
 }
