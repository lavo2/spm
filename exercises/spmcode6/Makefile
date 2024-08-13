# 
# FF_ROOT     points to the FastFlow root directory (i.e.
#             the one containing the ff directory).

# c++ -I. -I./include -I /Users/lavo/Desktop/Projects/spm/fastflow  -std=c++20 -DNO_CMAKE_CONFIG -Wall -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -O3 -finline-functions -DNDEBUG -o example-farm example-farm.cpp  -pthread
# g++ -std=c++20 -I. -I./include -I /Users/lavo/Desktop/Projects/spm/fastflow -Wall  -O3 -DNDEBUG -ffast-math -o example-farm example-farm.cpp -pthread

ifndef FF_ROOT 
#FF_ROOT		= ${HOME}/fastflow
FF_ROOT			= /Users/lavo/Desktop/Projects/spm/fastflow
endif

CXX                = g++-14 -std=c++20
OPTFLAGS	   = -O3 -DNDEBUG -ffast-math
CXXFLAGS          += -Wall -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER

INCLUDES	   = -I. -I./include -I $(FF_ROOT)
LIBS               = -pthread
SOURCES            = $(wildcard *.cpp)
TARGET             = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 

%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

all: $(TARGET)

example-omp:example-omp.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS) -fopenmp
example-omp2:example-omp2.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS) -fopenmp

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)
