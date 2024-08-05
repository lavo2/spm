CXX = g++
CXXFLAGS = -I . -O3 -mavx2
#-fopt-info-vec-all
#-fopt-info-vec-missed
#-fopt-info-vec-optimized

TARGET= one two three libf.so

all: $(TARGET)

libf.so: f.cpp
	$(CXX) -fPIC f.cpp -c -o f.o
	$(CXX) -shared f.o -o libf.so

three: three.cpp libf.so
	$(CXX) $(CXXFLAGS) -o $@ $< -Wl,-rpath,./ -L . -lf
clean:
	-rm -f *.o $(TARGET)

cleanall: clean
	-rm -f *~
