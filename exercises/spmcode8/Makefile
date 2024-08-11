CXXFLAGS            += -std=c++20
ifdef DEBUG
    OPTIMIZE_FLAGS  += -g -fno-inline-functions
else
    OPTIMIZE_FLAGS  += -O3 -ffast-math -DNDEBUG
endif
ifdef FF_HOME
    INCS            += -I$(FF_HOME)
else
    INCS            += -I ~/fastflow
endif

CXXFLAGS            += -Wall
LIBS                 = -pthread
INCLUDES             = $(INCS)

SOURCES              = $(wildcard *.cpp)
TARGET               = $(SOURCES:.cpp=)

.PHONY: all clean cleanall 
.SUFFIXES: .c .cpp .o

%.d: %.cpp
	set -e; $(CXX) -MM $(INCLUDES) $(CXXFLAGS) $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@
%.d: %.c
	set -e; $(CC) -MM $(INCLUDES) $(CFLAGS)  $< \
		| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
		[ -s $@ ] || rm -f $@
%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) -c -o $@ $<
%: %.cpp
	$(CXX) $(INCLUDES) $(CXXFLAGS) $(OPTIMIZE_FLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

all: $(TARGET)

ff_chol_mdf:CXXFLAGS+=-DDO_NOTHING
ff_chol_mdf:ff_chol_mdf.cpp


clean: 
	-rm -fr *.o *~
cleanall: clean
	-rm -fr $(TARGET) *.d ./socket*

include $(OBJS:.o=.d)
