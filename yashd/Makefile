yashd : yashd.o socket.o thread.o yash_cmd.o parse.o job.o process.o 
	gcc -std=c99 -lpthread -lreadline -g -o yashd yashd.o yash_cmd.o socket.o thread.o parse.o job.o process.o

yashd.o : yashd.c
	gcc -c yashd.c

socket.o : socket.c
	gcc -c socket.c

thread.o : thread.c
	gcc -c thread.c

yash_cmd.o : yash-holden/yash_cmd.c
	gcc -c yash-holden/yash_cmd.c

job.o : yash-holden/job.c
	gcc -c yash-holden/job.c

process.o : yash-holden/process.c
	gcc -c yash-holden/process.c

parse.o : yash-holden/parse.c
	gcc -c yash-holden/parse.c

clean :
	rm *.o yashd yash

yash:
	gcc -o yash yash.c socket.c -lreadline -std=c99