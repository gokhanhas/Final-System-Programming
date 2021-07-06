target:	final

final:	161044067_final.c 
		gcc -g 161044067_final.c graph.c queue.c cache.c graph.h queue.h cache.h -o server -lpthread -Wall
		gcc -g client.c -o client -Wall
clean:
		rm server
		rm client
