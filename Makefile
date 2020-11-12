CCFLAGS := -Wall -Werror

memdump: main.o
	$(CC) -o memdump main.o

clean:
	rm -rf memdump *.o
