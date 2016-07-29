CC=gcc -std=c99 -pthread -Wall

all: travel_client travel_server hashmap

# all targets begining with "travel_"
travel_%:  hashmap
	$(CC) $@.c $^.o -o bin/$@

hashmap:
	$(CC) -c c_hashmap/$@.c

clean:
	find . -name '*.o' -delete	
	find . -name '.*.sw*' -delete	
	find . -name '.*.py*' -delete	
