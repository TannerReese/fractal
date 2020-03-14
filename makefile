fractal: fractal.o
	gcc -o fractal fractal.o -lm -lncurses -lpng

fractal.o: fractal.c
	gcc -c -g -o fractal.o fractal.c


clean:
	rm *.o
