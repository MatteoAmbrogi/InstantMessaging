all: serv dev

serv: serv.c chat_message.h
	gcc -Wall serv.c -o serv

dev: dev.c chat_message.h
	gcc -Wall dev.c -o dev

clean:
	rm dev serv