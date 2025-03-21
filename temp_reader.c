//Simple Goke/Hisilicone temp reader, compile like mavfwd when needed

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <termios.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/util.h>


static char WfbLogFile[28]= "/tmp/board_temperature.msg";  

bool verbose = false;
 
static float last_board_temp;
 
static void *setup_temp_mem(off_t base, size_t size) {
	int mem_fd;

	mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
	if (mem_fd < 0) {
		fprintf(stderr, "can't open /dev/mem\n");
		return NULL;
	}

	char *mapped_area = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, base);
	if (mapped_area == MAP_FAILED) {
		fprintf(stderr, "read_mem_reg mmap error: %s (%d)\n", strerror(errno), errno);
		return NULL;
	}

	uint32_t MISC_CTRL45 = 0;

	// Set the T-Sensor cyclic capture mode by configuring MISC_CTRL45 bit[30]
	MISC_CTRL45 |= 1 << 30;

	// Set the capture period by configuring MISC_CTRL45 bit[27:20]
	// The formula for calculating the cyclic capture periodis as follows:
	//	T = N x 2 (ms)
	//	N is the value of MISC_CTRL45 bit[27:20]
	MISC_CTRL45 |= 50 << 20;

	// Enable the T-Sensor by configuring MISC_CTRL45 bit[31] and start to collect the temperature
	MISC_CTRL45 |= 1 << 31;

	*(volatile uint32_t *)(mapped_area + 0xB4) = MISC_CTRL45;

	return mapped_area;
}

static void temp_read(evutil_socket_t sock, short event, void *arg) {
	(void)sock;
	(void)event;
	char *mapped_area = arg;

	uint32_t val = *(volatile uint32_t *)(mapped_area + 0xBC);
	float tempo = val & ((1 << 16) - 1);
	tempo = ((tempo - 117) / 798) * 165 - 40;

	// only once
	//if (last_board_temp == -100)
	printf("Temp read: %f C\n", tempo);

	// Open the file in write mode, truncating it to zero size
    FILE *file = fopen(WfbLogFile, "w");
    if (file != NULL) {
        // Write the integer temperature to the file
        fprintf(file, "%d\n", (int)tempo);        
        fclose(file);
    } else         
        printf("Failed to open the file for writing.\n");
    

	last_board_temp = tempo;
}

static void signal_cb(evutil_socket_t fd, short event, void *arg)
{
	struct event_base *base = arg;
	(void)event;

	printf("%s signal received\n", strsignal(fd));
	event_base_loopbreak(base);
}


static int handle_data(	) {
	struct event_base *base = NULL;
	struct event *sig_int = NULL, *in_ev = NULL, *temp_tmr = NULL;
	int ret = EXIT_SUCCESS;
	
	base = event_base_new();

	sig_int = evsignal_new(base, SIGINT, signal_cb, base);
	event_add(sig_int, NULL);
	// it's recommended by libevent authors to ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);
 
	 
	void *mem = setup_temp_mem(0x12028000, 0xFFFF);
	temp_tmr = event_new(base, -1, EV_PERSIST, temp_read, mem);
	evtimer_add(temp_tmr, &(struct timeval){.tv_sec = 1});
 

	event_base_dispatch(base);

err:
	if (temp_tmr) {
		event_del(temp_tmr);
		event_free(temp_tmr);
	}
	 

	if (in_ev) {
		event_del(in_ev);
		event_free(in_ev);
	}

	if (sig_int)
		event_free(sig_int);

	if (base)
		event_base_free(base);

	libevent_global_shutdown();

	return ret;
}

int main(int argc, char **argv) {
	printf("Temp written to:%s\n", WfbLogFile);	 
	return handle_data();
}
