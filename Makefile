all: wt.o
	gcc -o wt wt.o -lcurl

clean: 
	rm *.o
	rm wt

wt.o: wt.cpp
	gcc -c wt.cpp 
