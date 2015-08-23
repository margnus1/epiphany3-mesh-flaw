struct counter {
    int counter;
    int actual;
};

#define TEST_SHARED_MEMORY 0

#if TEST_SHARED_MEMORY
#define INCREMENT_VECTOR_ADDR  ((volatile struct counter *)0x8f100000)
#define INCREMENT_CORE_STEP    (0x1000 / sizeof(struct counter))
#else
#define INCREMENT_VECTOR_ADDR  ((volatile struct counter *)0x2000)
#endif
