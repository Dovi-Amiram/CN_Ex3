all: sender receiver

receiver: Receiver.o
	gcc -Wall -g Receiver.o -o Receiver

sender: Sender.o
	gcc -Wall -g Sender.o -o Sender

Receiver.o: Receiver.c
	gcc -Wall -g -c Receiver.c

Sender.o: Sender.c
	gcc -Wall -g -c Sender.c

clean:
	rm -f *.o Sender Receiver