compare: compare.o qB.o qA.o linkedlist.o fNode.o
	gcc -g -std=c99 -Wvla -Wall -fsanitize=address,undefined -o compare compare.o qB.o qA.o linkedlist.o fNode.o -lm

compare.o: compare.c qB.h qA.h linkedlist.h
	gcc -c -g -std=c99 -Wvla -Wall -fsanitize=address,undefined compare.c

qB.o: qB.c qB.h
	gcc -c -g -std=c99 -Wvla -Wall -fsanitize=address,undefined qB.c

qA.o: qA.c qA.h
	gcc -c -g -std=c99 -Wvla -Wall -fsanitize=address,undefined qA.c

fNode.o: fNode.c fNode.h
	gcc -c -g -std=c99 -Wvla -Wall -fsanitize=address,undefined fNode.c

linkedlist.o: linkedlist.c linkedlist.h
	gcc -c -g -std=c99 -Wvla -Wall -fsanitize=address,undefined linkedlist.c

clean:
	rm -f *.o $(OUTPUT) compare
