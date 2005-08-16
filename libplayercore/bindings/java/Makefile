JAVA_INCLUDES = -I/usr/lib/j2sdk1.5-sun/include -I/usr/lib/j2sdk1.5-sun/include/linux

all: libplayercore_java.so
	javac *.java

libplayercore_java.so: playercore_java_wrap.cc
	g++ -c $(JAVA_INCLUDES) -I../../.. $<
	g++ -shared -o $@ playercore_java_wrap.o ../../*.o ../../../server/libplayerdrivers/*.o -lltdl

playercore_java_wrap.cc: playercore_java.i ../../player.h
	swig -v -java -c++ -o $@ -I../../.. $<

clean:
	rm -f *.o *.so *.class playercore_java_wrap.cc SWIGTYPE* player*.java
