smallsh: smallsh.o input.o
	gcc -Wall -Wextra -pedantic -o smallsh input.o smallsh.o 
smallsh.o: smallsh.c smallsh.h
	gcc -Wall -Wextra -pedantic -c smallsh.c
input.o: input.c smallsh.h
	gcc -Wall -Wextra -pedantic -c input.c
clean:
	rm smallsh input.o smallsh.o
valgrind: 
	valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./smallsh 
