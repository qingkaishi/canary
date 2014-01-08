SHELL=/bin/sh

# Compile
all:
	gcc -c -march=native Tracer.cpp -I../../include
	ar rcs libtrace.a Tracer.o
	cp *.a ../../Release+Asserts/lib/

clean:
	if [ -f Tracer.o ]; then rm Tracer.o; fi
	if [ -f libtrace.a ]; then rm libtrace.a; fi

install: all
	cp *.a /usr/local/lib/

uninstall:
	rm /usr/local/lib/libtrace.a
