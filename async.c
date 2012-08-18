/*
 * Simple libusb-1.0 benchmark programm
 * It openes an USB device, expects two Bulk endpoints,
 *   EP1 should be IN
 *   EP2 should be OUT
 * It uses Asynchronous device I/O
 *
 * Compile:
 *   gcc -lusb-1.0 -lrt -o async async.c
 * Run:
 *   ./async
 * As far as I know, sys/time.h is POSIX only and not available on Windows.
 * It should be trivial to exchange for a precise Windows time API.
 * For Documentation on libusb see:
 *   http://libusb.sourceforge.net/api-1.0/modules.html

 * Tweaked/mutated/altered/hacked from:
 * http://libusb.6.n5.nabble.com/porting-from-0-1-to-0-93-example-code-tp5589p5593.html
 */

#include <signal.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>


#define USB_VENDOR_ID	    0x0483      /* USB vendor ID used by the device
                                         * 0x0483 is STMs ID
                                         */
#define USB_PRODUCT_ID	    0xFFFF      /* USB product ID used by the device */
#define USB_ENDPOINT_IN	    (LIBUSB_ENDPOINT_IN  | 1)   /* endpoint address */
#define USB_ENDPOINT_OUT	(LIBUSB_ENDPOINT_OUT | 2)   /* endpoint address */


//Global variables:
struct libusb_device_handle *devh = NULL;
#define LEN_IN_BUFFER 1024*8
static uint8_t in_buffer[LEN_IN_BUFFER];

// OUT-going transfers (OUT from host PC to USB-device)
struct libusb_transfer *transfer_out = NULL;

// IN-coming transfers (IN to host PC from USB-device)
struct libusb_transfer *transfer_in = NULL;

static libusb_context *ctx = NULL;

int do_exit = 0;

// Function Prototypes:
void sighandler(int signum);
void print_libusb_transfer(struct libusb_transfer *p_t);

uint32_t benchPackets=1;
uint32_t benchBytes=0;
struct timespec t1, t2;
uint32_t diff=0;

enum {
    out_deinit,
    out_release,
    out
} exitflag;

// Out Callback
//   - This is called after the Out transfer has been received by libusb
void cb_out(struct libusb_transfer *transfer)
{
	fprintf(stderr, "cb_out: status =%d, actual_length=%d\n",
		transfer->status, transfer->actual_length);
}

// In Callback
//   - This is called after the command for version is processed.
//     That is, the data for in_buffer IS AVAILABLE.
void cb_in(struct libusb_transfer *transfer)
{
    //measure the time
	clock_gettime(CLOCK_REALTIME, &t2);
	//submit the next transfer
	libusb_submit_transfer(transfer_in);

	benchBytes += transfer->actual_length;
	//this averages the bandwidth over many transfers
	if(++benchPackets%100==0){
		//Warning: uint32_t has a max value of 4294967296 so this will overflow over 4secs
		diff = (t2.tv_sec-t1.tv_sec)*1000000000L+(t2.tv_nsec-t1.tv_nsec);
		t1.tv_sec = t2.tv_sec;
	 	t1.tv_nsec = t2.tv_nsec;
	 	printf("\rreceived %5d transfers and %8d bytes in %8d us, %8.1f B/s", benchPackets, benchBytes, diff/1000, benchBytes*1000000.0/(diff/1000));
		fflush(stdout);
		benchPackets=0;
		benchBytes=0;
	}
}

int main(void)
{
	struct sigaction sigact;

	int r = 1;  // result
	int i;

	//init libUSB
	r = libusb_init(NULL);
	if (r < 0) {
		fprintf(stderr, "Failed to initialise libusb\n");
		return 1;
	}

    //open the device
	devh = libusb_open_device_with_vid_pid(ctx,
 		USB_VENDOR_ID, USB_PRODUCT_ID);
 	if (!devh) {
 		perror("device not found");
 		return 1;
 	}

    //claim the interface
	r = libusb_claim_interface(devh, 0);
	if (r < 0) {
		fprintf(stderr, "usb_claim_interface error %d\n", r);
		exitflag = out;
		do_exit = 1;
	} else  {
        printf("Claimed interface\n");

        // allocate transfer of data IN (IN to host PC from USB-device)
        transfer_in  = libusb_alloc_transfer(0);
        libusb_fill_bulk_transfer( transfer_in, devh, USB_ENDPOINT_IN,
            in_buffer,  LEN_IN_BUFFER,  // Note: in_buffer is where input data written.
            cb_in, NULL, 0); // no user data

        //take the initial time measurement
        clock_gettime(CLOCK_REALTIME, &t1);
        //submit the transfer, all following transfers are initiated from the CB
        r = libusb_submit_transfer(transfer_in);

        // Define signal handler to catch system generated signals
        // (If user hits CTRL+C, this will deal with it.)
        sigact.sa_handler = sighandler;  // sighandler is defined below. It just sets do_exit.
        sigemptyset(&sigact.sa_mask);
        sigact.sa_flags = 0;
        sigaction(SIGINT, &sigact, NULL);
        sigaction(SIGTERM, &sigact, NULL);
        sigaction(SIGQUIT, &sigact, NULL);

        printf("Entering loop to process callbacks...\n");
    }

	/* The implementation of the following while loop makes a huge difference.
	 * Since libUSB asynchronous mode doesn't create a background thread,
	 * libUSB can't create a callback out of nowhere. This loop calls the event handler.
	 * In real applications you might want to create a background thread or call the event
	 * handler from your main event hanlder.
	 * For a proper description see:
	 * http://libusbx.sourceforge.net/api-1.0/group__asyncio.html#asyncevent
	 * http://libusbx.sourceforge.net/api-1.0/group__poll.html
	 * http://libusbx.sourceforge.net/api-1.0/mtasync.html
	 */
    if(1){
        // This implementation uses a blocking call
        while (!do_exit) {
            r =  libusb_handle_events_completed(ctx, NULL);
            if (r < 0){   // negative values are errors
                exitflag = out_deinit;
                break;
            }
        }
    }
    else{
        // This implementation uses a blocking call and aquires a lock to the event handler
        struct timeval timeout;
		timeout.tv_sec  = 0;       // seconds
		timeout.tv_usec = 100000;  // ( .1 sec)
        libusb_lock_events(ctx);
        while (!do_exit) {
            r = libusb_handle_events_locked(ctx, &timeout);
         	if (r < 0){   // negative values are errors
                exitflag = out_deinit;
                break;
         	}
		}
        libusb_unlock_events(ctx);
    }

	// If these transfers did not complete then we cancel them.
	// Unsure if this is correct...
	if (transfer_out) {
		r = libusb_cancel_transfer(transfer_out);
		if (0 == r){
			printf("transfer_out successfully cancelled\n");
		}
		if (r < 0){
		    exitflag = out_deinit;
		}

	}
	if (transfer_in) {
		r = libusb_cancel_transfer(transfer_in);
		if (0 == r){
			printf("transfer_in successfully cancelled\n");
		}
		if (r < 0){
		    exitflag = out_deinit;
		}
	}

    switch(exitflag){
    case out_deinit:
        printf("at out_deinit\n");
        libusb_free_transfer(transfer_out);
        libusb_free_transfer(transfer_in);

    case out_release:
        libusb_release_interface(devh, 0);
    case out:
        libusb_close(devh);
        libusb_exit(NULL);
    }
	return 0;
}


// This will catch user initiated CTRL+C type events and allow the program to exit
void sighandler(int signum)
{
	printf("sighandler\n");
	do_exit = 1;
}


// debugging function to display libusb_transfer
void print_libusb_transfer(struct libusb_transfer *p_t)
{	int i;
	if ( NULL == p_t){
		printf("No libusb_transfer...\n");
	}
	else {
		printf("libusb_transfer structure:\n");
		printf("flags   =%x \n", p_t->flags);
		printf("endpoint=%x \n", p_t->endpoint);
		printf("type    =%x \n", p_t->type);
		printf("timeout =%d \n", p_t->timeout);
		// length, and buffer are commands sent to the device
		printf("length        =%d \n", p_t->length);
		printf("actual_length =%d \n", p_t->actual_length);
		printf("buffer    =%p \n", p_t->buffer);

		for (i=0; i < p_t->length; i++){
			printf(" %x", i, p_t->buffer[i]);
		}
	}
	return;
}


