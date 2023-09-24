# Multi-Process Word Counter

This is a C project that demonstrates the advantages of using multi-process programming. The project a progam `wc_mul.c`, the program simply counts words in a text file using multi process programming.

`wc_mul.c` performs inter-process communication using pipes. Every child takes in a portion of the .txt file and counts words within that portion. I have created an array of pipes where each child can write the result of their word count into their respective pipe so that the parent can later read all of the results.

## Environment

- **Operating System**: Linux
- **Kernel Version**: 3.10.0-1160.42.2.el7.x86_64
- **C Compiler**: GCC 10.1.0

## Usage

To compile the project, run:

```shell
make clean
make all
```

### Single-Process Word Counter

To count words using a single process, run:

```shell
./wc_mul 1 large.txt
```

you will see that with only using a single process, it takes a bit of time for the program to complete.

### Multi-Process Word Counter

To use multi-process programming and see a significant speedup, run:
./wc_mul <num of processes> large.txt
example:

```shell
./wc_mul 10 large.txt
```

this will run the multiprocess word counting program using 10 processes, experiment with different number of processes to see different speeds. The maximum allowed processes in this program is 100.

The `large.txt` file contains over 100mb of words and can be used to see the significant speed difference in action.

You will notice that compared to only using a single process, the `wc_mul.c` program runs significantly faster with more processes running.
When timed, without multi process, the program runs at real 0m1.099s. With 10 processes, the program runs at real 0m0.129s.

Thank you :)
