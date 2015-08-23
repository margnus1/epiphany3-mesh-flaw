#include <stdlib.h>
#include <limits.h>
#include <e-lib.h>
#include "addresses.h"

int coreno() {
    int row, col;
    e_coords_from_coreid(e_get_coreid(), &row, &col);
    return col + row * e_group_config.group_cols;
}

int
main(int argc, char **argv)
{
    int core = coreno();
    for (int i = 0; i < INT_MAX; i++) {
	INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP] = i;
	__asm__("nop");
	int got_back = INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP];
	if (got_back != i) {
	    INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP] = -i;
	    INCREMENT_VECTOR_ADDR[core * INCREMENT_CORE_STEP + 1] = got_back;
	    exit(1);
	}
    }
}
