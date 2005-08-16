#JAVA_INCLUDES = -I/usr/lib/j2sdk1.5-sun/include -I/usr/lib/j2sdk1.5-sun/include/linux
JAVA_INCLUDES = -I/System/Library/Frameworks/JavaVM.framework/Versions/1.5.0/Headers

all: libplayerxdr_java.so
	javac *.java

libplayerxdr_java.so: playerxdr_java_wrap.c ../../playerxdr.c
	gcc -c $(JAVA_INCLUDES) -I../../../ playerxdr_java_wrap.c
	gcc -Wall -c -I../../../ ../../playerxdr.c
	gcc -shared -o $@ playerxdr_java_wrap.o playerxdr.o

playerxdr_java_wrap.c: playerxdr_java.i ../../playerxdr.h ../../../libplayercore/player.h
	swig -v -java -I../../.. $<

clean:
	rm -f *.o *.so *.class *.c SWIGTYPE* player*.java
