FLAGS=


fractal: fractal_main.o fractal.o
	gcc $(FLAGS) -o fractal fractal_main.o fractal.o -lm -lncurses -lpng

fractal_main.o: fractal_main.c fractal.h
	gcc -c $(FLAGS) -o fractal_main.o fractal_main.c

fractal.o: fractal.c fractal.h
	gcc -c $(FLAGS) -o fractal.o fractal.c


buddha: buddha_main.o buddha.o fractal.o
	gcc $(FLAGS) -o buddha buddha_main.o buddha.o fractal.o -lm -lncurses -lpng

buddha_main.o: buddha_main.c buddha.h
	gcc -c $(FLAGS) -o buddha_main.o buddha_main.c

buddha.o: buddha.c buddha.h
	gcc -c $(FLAGS) -o buddha.o buddha.c


clean:
	rm -f *.o  # Remove Object files
	rm -f fractal ; rm -f buddha  # Remove binaries

