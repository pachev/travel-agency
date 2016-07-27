all: travel_client travel_server

travel_%: 
	gcc src/$@.c -o bin/$@
