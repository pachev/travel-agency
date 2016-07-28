all: travel_client travel_server hash_map

# all targets begining with "travel_"
travel_%:  hash_map
	gcc $@.c $^.o -o bin/$@

hash_map:
	gcc -c $@.c

clean:
	find . -name '*.o' -delete	
