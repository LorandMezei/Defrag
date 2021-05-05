defrag: defrag.o
	gcc defrag.o -o defrag -lpthread

defrag.o: defrag.c
	gcc -c defrag.c -o defrag.o -lpthread

clean:
	rm -f defrag.o defrag