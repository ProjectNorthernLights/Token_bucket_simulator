warmup2: main.o MemoryManager.o token_bucket.o my402list.o
	gcc -o warmup2 -g main.o MemoryManager.o token_bucket.o my402list.o -pthread

main.o: main.c token_bucket.h MemoryManager.h
	gcc -o main.o -pthread -g -c -Wall main.c

token_bucket.o: token_bucket.c token_bucket.h my402list.h
	gcc -o token_bucket.o -g -c -Wall token_bucket.c

MemoryManager.o: MemoryManager.c MemoryManager.h
	gcc -o MemoryManager.o -g -c -Wall MemoryManager.c

my402list.o: my402list.c my402list.h cs402.h
	gcc -o my402list.o -g -c -Wall my402list.c

clean:
	rm -f *.o warmup2
