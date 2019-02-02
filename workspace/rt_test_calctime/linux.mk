all: linux-bin/test_calctime

linux-bin/test_calctime: linux-bin/test_calctime.o
	cd linux-bin && gcc -o test_calctime test_calctime.o -lm

linux-bin/test_calctime.o: test_calctime.c linux-bin
	gcc -c test_calctime.c ; cp test_calctime.o linux-bin

linux-bin:
	mkdir linux-bin 2>/dev/null; true
	
clean:
	cd linux-bin && rm *.o test_calctime
