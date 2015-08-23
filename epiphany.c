#include <stdlib.h>
#include <limits.h>
#include <e-lib.h>
#include "addresses.h"

int coreno() {
    int row, col;
    e_coords_from_coreid(e_get_coreid(), &row, &col);
    return col + row * e_group_config.group_cols;
}

volatile struct counter *get_mem() {
#if TEST_SHARED_MEMORY
    int core = coreno();
    return &INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP];
#else
    int row, col;
    e_coords_from_coreid(e_get_coreid(), &row, &col);
    col = e_group_config.group_cols - 1 - col;
    row = e_group_config.group_rows - 1 - row;
    return e_get_global_address(row, col, (void*)INCREMENT_VECTOR_ADDR);
#endif
}

#define WRITE_AMPLIFICATION 100

int main(int argc, char **argv) {
    volatile struct counter *mem = get_mem();
    for (int i = 0; i < INT_MAX; i++) {

	mem->counter = i;

	int got_back = mem->counter;
	if (got_back != i) {
	    mem->actual = got_back;
	    mem->counter = -i;
	    exit(1);
	}

	/* For whatever reason, a busy mesh makes the problem reproduce less
	 * frequently with shared memory, but more frequently with remote local
	 * memory */
#if !TEST_SHARED_MEMORY
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
	mem->counter = i;
#endif
    }
}
