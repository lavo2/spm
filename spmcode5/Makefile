CXX                = g++ -std=c++20
OPTFLAGS	   = -O3
CXXFLAGS          += -Wall 
AUTOFLAGS          = -march=native -ffast-math -mavx2
INCLUDES	   = -I. -I./include
LIBS               = -pthread -fopenmp
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

all: $(TARGET)

omp_all_pair:omp_all_pair.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(AUTOFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)
