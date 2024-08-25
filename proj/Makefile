# 
# FF_ROOT     pointing to the FastFlow root directory (i.e.
#             the one containing the ff directory).
ifndef FF_ROOT
FF_ROOT		= ../fastflow
endif

CXX		= g++ -std=c++20
CXXMPI      = mpicxx -std=c++20
INCLUDES	= -I . -I miniz
CXXFLAGS  	+= -Wall -DNO_DEFAULT_MAPPING -DBLOCKING_MODE -DFF_BOUNDED_BUFFER

LDFLAGS 	= -pthread
OPTFLAGS	= -O3 -ffast-math -DNDEBUG

TARGETS		= 	mainffa2a \
		  		mainseq \
		  		mainmpi

.PHONY: all clean cleanall
.SUFFIXES: .cpp 


%: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)

all		: $(TARGETS)

mainseq	: mainseq.cpp cmdline.hpp utility.hpp
	$(CXX) $(INCLUDES) -I$(FF_ROOT) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c

#mainff       : mainff.cpp utility.hpp cmdline.hpp
#	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(FF_ROOT) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)

mainffa2a       : mainffa2a.cpp utility.hpp cmdlinea2a.hpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(FF_ROOT) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)

mainmpi      : mainmpi.cpp utilitympi.hpp cmdlinempi.hpp
	$(CXXMPI) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)

mainmpistatus      : mainmpistatus.cpp utilitympi.hpp cmdlinempi.hpp
	$(CXXMPI) $(CXXFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< ./miniz/miniz.c $(LDFLAGS)



clean		: 
	rm -f $(TARGETS) 
cleanall	: clean
	\rm -f *.o *~



