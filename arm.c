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
#define READ(CORENO, PTR) read_mem(CORENO, PTR)
int read_mem(int coreno, volatile void *ptr) {
    int word;
    ssize_t amount;
    if ((unsigned)ptr > EXTERNAL_BASE) {
	amount = e_read(&memory, 0, 0, (off_t)ptr - EXTERNAL_BASE, &word,
			sizeof(word));
    } else {
	int row, col;
	e_get_coords_from_num(&workgroup, coreno, &row, &col);
	amount = e_read(&workgroup, row, col, (off_t)ptr, &word, sizeof(word));
    }
    assert(amount == sizeof(word));
    return word;
}

#define WRITE(CORENO, PTR, DATA) write_mem(CORENO, PTR, DATA)
void write_mem(int coreno, volatile void *ptr, int data) {
    ssize_t amount;
    if ((unsigned)ptr > EXTERNAL_BASE) {
	amount = e_write(&memory, 0, 0, (off_t)ptr - EXTERNAL_BASE, &data,
			 sizeof(data));
    } else {
	int row, col;
	e_get_coords_from_num(&workgroup, coreno, &row, &col);
	amount = e_read(&workgroup, row, col, (off_t)ptr, &data, sizeof(data));
    }
    assert(amount == sizeof(data));
}

void cleanup() {
    int ret;
    ret = e_finalize();
    if (ret != E_OK) perror("e_finalize");
}

int memory_online = 0;
void exit_with(const char *msg, int status) {
    fprintf(stderr, "%s\n", msg);
    cleanup();
    exit(status);
}

void interrupted() {
    exit_with("Exiting! ", 130);
}

#define WIDTH 4
#define HEIGHT 4
#define COUNT (WIDTH*HEIGHT)
volatile struct counter *get_mem(int core) {
#if TEST_SHARED_MEMORY
    return &INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP];
#else
    /*
     * We don't need to repeat the coordinate mirroring here; it's not important
     * what order we read the data.
     */
    return INCREMENT_VECTOR_ADDR;
#endif
}

void zero_vector() {
    for (int i = 0; i < COUNT; i++) {
	WRITE(i, &get_mem(i)->counter, 0);
    }
    BARRIER();
}

/*
 * Return -1 if all cores are done and OK.
 * Return i+1 if core i detected a problem.
 */
int check_for_result() {
    int done = 0, result = 0;
    for (int i = 0; i < COUNT; i++) {
	volatile struct counter *mem = get_mem(i);
	int val = READ(i, &mem->counter);
	printf("%10d ", val);
	/* The cores set mem->counter to -i when they detect a problem */
	if (val < 0) {
	    result = i+1;
	    printf(" (%d)", READ(i, &mem->actual));
	}
	if (val == INT_MAX - 1) done++;
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

    fprintf(stderr, "Testing address %p\n", get_mem(0));

    zero_vector();

    ret = e_load_group(binary, &workgroup, 0, 0, WIDTH, HEIGHT, E_TRUE);
    if (ret != E_OK) { perror("e_load"); cleanup(); return 1; }

    while(1) {
	int r = check_for_result();
	if (r > 0) {
	    exit_with("A core detected a memory inconsistency", r);
	} else if (r < 0) {
	    exit_with("All cores finished OK", 0);
	}
	sleep(1);
    }
}
