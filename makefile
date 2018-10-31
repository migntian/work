.PHONY:clean
server:http.c listensock.c
	gcc -o $@ $^ -lpthread
clean:
	rm server 
