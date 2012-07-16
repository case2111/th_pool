arping_test:threads_pool.o threads_arping.o
	gcc -pthread threads_arping.o threads_pool.o -o arping_test
threads_arping.o:threads_arping.c threads_arping.h threads_pool.h
	gcc -c threads_arping.c -o threads_arping.o
threads_pool.o:threads_pool.c threads_pool.h
	gcc -c -pthread threads_pool.c -o threads_pool.o
clean:
	rm -rf *.o arping_test