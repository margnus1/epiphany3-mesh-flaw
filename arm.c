#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <limits.h>
#include <e-hal.h>
#include <e-loader.h>
#include "addresses.h"

#define BARRIER() __asm__("DMB")

e_epiphany_t workgroup;
e_mem_t      memory;

#define EXTERNAL_BASE    0x8e000000
#define READ(PTR) read_mem(PTR)
int read_mem(volatile void *ptr) {
    int word;
    ssize_t amount;
    if ((unsigned)ptr > EXTERNAL_BASE) {
	amount = e_read(&memory, 0, 0, (off_t)ptr - EXTERNAL_BASE, &word,
			sizeof(word));
    } else {
	amount = e_read(&workgroup, 0, 0, (off_t)ptr, &word, sizeof(word));
    }
    assert(amount == sizeof(word));
    return word;
}

//#define WRITE(PTR, DATA) ee_write_word(&memory, 0, 0, ((unsigned)PTR) - EXTERNAL_BASE, DATA)
#define WRITE(PTR, DATA) write_mem(PTR, DATA)
void write_mem(volatile void *ptr, int data) {
    ssize_t amount;
    if ((unsigned)ptr > EXTERNAL_BASE) {
	amount = e_write(&memory, 0, 0, (off_t)ptr - EXTERNAL_BASE, &data,
			 sizeof(data));
    } else {
	amount = e_read(&workgroup, 0, 0, (off_t)ptr, &data, sizeof(data));
    }
    assert(amount == sizeof(data));
}

const time_t TIMEOUT = 2;

void cleanup() {
    int ret;
    ret = e_finalize();
    if (ret != E_OK) perror("e_finalize");
}

int memory_online = 0;
void exit_with(const char *msg, int status) {
    /* if (memory_online) */
    /* 	fprintf(stderr, */
    /* 		"%sread=%d, write=%d, flag=%d, pc=%#x, status=%#x\n", */
    /* 		msg, */
    /* 		READ(MAILBOX_READ_PTR), READ(MAILBOX_WRITE_PTR), */
    /* 		READ(EVENT_FLAG_PTR), READ(E_REG_PC), READ(E_REG_STATUS)); */
    /* else  */fprintf(stderr, "%s\n", msg);
    cleanup();
    exit(status);
}

void interrupted() {
    exit_with("Exiting! ", 130);
}

#define WIDTH 4
#define HEIGHT 4
#define COUNT (WIDTH*HEIGHT)
void zero_vector() {
    for (int i = 0; i < COUNT; i++) {
	WRITE(INCREMENT_VECTOR_ADDR + i * INCREMENT_CORE_STEP, 0);
    }
    BARRIER();
}

int check_for_result() {
    int done = 0, result = 0;
    for (int i = 0; i < COUNT; i++) {
	int val = READ(INCREMENT_VECTOR_ADDR + i * INCREMENT_CORE_STEP);
	printf("%10d ", val);
	if (val < 0) {
	    result = i;
	    printf(" (%d)", READ(INCREMENT_VECTOR_ADDR
				 + i * INCREMENT_CORE_STEP + 1));
	}
	if (val == INT_MAX) done++;
    }
    printf("\n");
    if (done == COUNT) result = -1;
    return result;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
	fprintf(stderr, "Usage: %s [epiphany binary]\n", argv[0]);
	exit(1);
    }
    char *binary = argv[1];

    int ret;
    signal(SIGINT, interrupted);
    ret = e_init(NULL);
    if (ret != E_OK) { return 1; }

    e_reset_system();
    if (ret != E_OK) { perror("e_reset_system"); cleanup(); return 1; }

    ret = e_open(&workgroup, 0, 0, WIDTH, HEIGHT);
    if (ret != E_OK) { perror("e_open"); cleanup(); return 1; }

    ret = e_alloc(&memory, 0, 0x02000000 - 4);
    if (ret != E_OK) { perror("e_alloc"); cleanup(); return 1; }
    memory_online = 1;

    zero_vector();

    ret = e_load_group(binary, &workgroup, 0, 0, WIDTH, HEIGHT, E_TRUE);
    if (ret != E_OK) { perror("e_load"); cleanup(); return 1; }

    while(1) {
	int r = check_for_result();
	if (r > 0) {
	    exit_with("A core detected a memory inconsistency ", r);
	} else if (r < 0) {
	    exit_with("All cores finished OK ", 0);
	}
	sleep(1);
    }
}
