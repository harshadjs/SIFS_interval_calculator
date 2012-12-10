# Modify CC for cross compilation
CC=gcc

all:
	$(CC) sifs_calculator.c -o sifs_calculator
clean:
	rm sifs_calculator
