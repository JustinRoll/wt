all: wt.o
	gcc -g -o wt wt.o -lcurl

clean: 
	rm *.o
	rm wt

wt.o: wt.cpp
	gcc -g -c wt.cpp 
