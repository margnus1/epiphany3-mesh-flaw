Epiphany Mesh Network Defect
============================

This repository contains a simple program that demonstrates a flaw in the
implementation of the mesh network in the Epiphany-III.

The flaw is that read and write transactions to the same memory address are
allowed to overtake each other in the mesh network. This means that a read
following a write to the same non-local address might return stale data,
breaking assumptions of programmers, compilers, and the C language itself. This
behaviour also contradicts the second guarantee about access to non-local memory
in section "4.2 Memory Order Model" of the Epiphany Architecture Reference.

Essentially, the demonstration boils down to

    for (int i = 0; i < INT_MAX; i++) {
        mem->counter = i;
        int got_back = mem->counter;
        if (got_back != i) {
            FAIL();
        }
    }

This program does eventually fail for addresses `mem` both on other cores, and
in external RAM.

How to run it
-------------

    make && ./arm fast.srec

If you're running a really old disk image for the Parallella, you might need to
run it with sudo instead:

    make && sudo -E LD_LIBRARY_PATH=$LD_LIBRARY_PATH ./arm fast.srec

To test shared memory instead, please modify `address.h`.

The program will terminate either when an inconsistency is found, or when all
cores have performed `INT_MAX` iterations, printing either "A core detected a
memory inconsistency" or "All cores finished OK".
