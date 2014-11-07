Iterative-Parallel-Mergesort
============================
A quick multithreaded iterative mergesort in C

Give the parallel method a bunch of numbers to sort and a number of threads to open.
It breaks up the list of numbers into a tree of merging operations, with full mergesorts at the leaves of the tree.

The serial version it uses appears to be about 20% faster than C's built in qsort() on ints.
The parallel version's speed up obviously depends on how many processing units your computer has.

But sorting 2^25 random numbers went like this on my machine(with 2 hyperthreaded cores, 4 processing units):
Parallel: 1.670886
Serial: 4.179196
Built in: 5.130592

============================
Build it with:
gcc iterative_mergesort.c -lpthread -lm -lrt -msse -msse2 -msse3 -O3

Run it with:
./a.out
