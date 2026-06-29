all: server client

server: threadpool.c database.c tcpserver.c
	gcc -Wall -Wextra -pthread -g $^ -o $@

client: tcpclient.c 
	gcc -Wall -Wextra -pthread -g $^ -o $@

clean: server client
	rm -f server client