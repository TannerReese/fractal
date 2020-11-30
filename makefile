FLAGS=

fractal: fractal_main.o fractal.o
	gcc $(FLAGS) -o fractal fractal_main.o fractal.o -lm -lncurses -lpng

fractal_main.o: fractal_main.c fractal.h
	gcc -c $(FLAGS) -o fractal_main.o fractal_main.c

fractal.o: fractal.c fractal.h
	gcc -c $(FLAGS) -o fractal.o fractal.c

clean:
	rm *.o
