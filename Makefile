all: shm_server

shm_server: shm_server.o
		gcc -o shm_server shm_server.o -lsndfile -lportaudio

shm_server.o: shm_server.c
		gcc -c -Wall shm_server.c

clean: rm shm_server shm_server.o
