CXX	   +=-std=c++20
INCLUDES    = -I ./dependencies

ifdef DEBUG
    OPTIMIZE_FLAGS  += -g
else
    OPTIMIZE_FLAGS  += -O3 -DNDEBUG
endif

OBJS            = dependencies/frechet_distance.o dependencies/frechet_distance2.o dependencies/geometry_basics.o
TARGETS		= lshseq

.PHONY: all clean cleanall
.SUFFIXES: .cpp 

lshseq: lshseq.cpp $(OBJS) dependencies/frechet_distance.hpp dependencies/geometry_basics.hpp
%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTIMIZE_FLAGS) -o $@ $< $(OBJS) $(LDFLAGS) $(LIBS)

all		: $(TARGETS)
clean		: 
	rm -f $(TARGETS)
cleanall	: clean
	rm -f *.o *~ dependencies/*.o dependencies/*~
