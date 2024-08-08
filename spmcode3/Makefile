CXX                = g++ -std=c++20
OPTFLAGS	   = -O3
AVXFLAGS           = -mavx2
AUTOFLAGS          = -march=native -ffast-math -mavx2
CXXFLAGS          += -Wall 
INCLUDES	   = -I. -I./include
LIBS               = -pthread
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

all: $(TARGET)

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)
