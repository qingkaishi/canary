SHELL=/bin/sh

# Compile
all: LeapRecorder.cpp CanaryRecorder.cpp Replayer.cpp
	gcc -c -march=native LeapRecorder.cpp  -mrtm
	gcc -c -march=native CanaryRecorder.cpp -mrtm
	gcc -c -march=native Replayer.cpp  -mrtm
	ar rcs libcanaryrecord.a CanaryRecorder.o
	ar rcs libleaprecord.a LeapRecorder.o
	ar rcs libreplay.a Replayer.o
	cp *.a ../../Release+Asserts/lib/

for-debug: LeapRecorder.cpp CanaryRecorder.cpp Replayer.cpp
	gcc -c -DDEBUG -march=native LeapRecorder.cpp  -mrtm
	gcc -c -DDEBUG -march=native CanaryRecorder.cpp  -mrtm
	gcc -c -DDEBUG -march=native Replayer.cpp  -mrtm
	ar rcs libcanaryrecord.a CanaryRecorder.o
	ar rcs libleaprecord.a LeapRecorder.o
	ar rcs libreplay.a Replayer.o
	cp *.a ../../Release+Asserts/lib/

clean:
	if [ -f CanaryRecorder.o ]; then rm CanaryRecorder.o; fi
	if [ -f LeapRecorder.o ]; then rm LeapRecorder.o; fi
	if [ -f Replayer.o ]; then rm Replayer.o; fi
	if [ -f libcanaryrecord.a ]; then rm libcanaryrecord.a; fi
	if [ -f libleaprecord.a ]; then rm libleaprecord.a; fi
	if [ -f libreplay.a ]; then rm libreplay.a; fi

install: all
	cp *.a /usr/local/lib/

uninstall:
	rm /usr/local/lib/libleaprecord.a
	rm /usr/local/lib/libcanaryrecord.a
	rm /usr/local/lib/libreplay.a
