FLAGS=

fractal: fractal.o
	gcc $(FLAGS) -o fractal fractal.o -lm -lncurses -lpng

fractal.o: fractal.c
	gcc -c $(FLAGS) -o fractal.o fractal.c


clean:
	rm *.o
