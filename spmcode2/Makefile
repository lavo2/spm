CXX                = g++ -std=c++17
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

matrix_mult_avx: matrix_mult_avx.cpp
	$(CXX) $(INCLUDES) $(AVXFLAGS) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)
vector_norm_aos_avx: vector_norm_aos_avx.cpp
	$(CXX) $(INCLUDES) $(AVXFLAGS) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)
vector_norm_soa_avx: vector_norm_soa_avx.cpp
	$(CXX) $(INCLUDES) $(AVXFLAGS) $(CXXFLAGS) $(OPTFLAGS) -o $@ $< $(LIBS)

clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET)
