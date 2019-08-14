# To make the library
librsocket.a: rsocket.o
	ar rcs librsocket.a rsocket.o

rsocket.o: rsocket.c
	gcc -c rsocket.c -pthread

clear:
	rm librsocket.a rsocket.o

# Below lines are for user1 and user2 

# To make user1.c executable to us1, run '$ make us1' 
us1: user1.o librsocket.a 
	gcc -o us1 user1.o -L. -lrsocket -pthread  

us2: user2.o librsocket.a
	gcc -o us2 user2.o -L. -lrsocket -pthread 

user2.o: user2.c
	gcc -c user2.c -pthread

user1.o: user1.c
	gcc -c user1.c -pthread

# To clean the socket if connection got closed improperly
sockcln1:
	fuser -k 110058/udp

sockcln2:
	fuser -k 110059/udp

rmall:
	rm librsocket.a rsocket.o user1.o user2.o us1 us2