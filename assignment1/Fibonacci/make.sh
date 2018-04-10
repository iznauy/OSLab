#!/bin/sh
nasm -felf64 -o Fibonacci.o Fibonacci.asm
gcc -m64 -o Fibonacci Fibonacci.o
./Fibonacci
