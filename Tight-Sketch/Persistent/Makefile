EXEC += main_hitter main_changer main_simd
all: $(EXEC)

CFLAGS = -Wall -std=c++11 -O3
HEADER += hash.h datatypes.hpp util.h adaptor.hpp 
SRC += hash.c adaptor.cpp
SKETCHHEADER += tightsketch.hpp
SKETCHSRC += tightsketch.cpp
LIBS= -lpcap 

main_changer: main_changer.cpp $(SRC) $(HEADER) $(SKETCHHEADER)
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SRC) $(SKETCHSRC) $(LIBS)

main_hitter: main_hitter.cpp $(SRC) $(HEADER) $(SKETCHHEADER) 
	g++ $(CFLAGS) $(INCLUDES) -o $@ $< $(SRC) $(SKETCHSRC) $(LIBS) 

main_simd: main_simd.cpp $(SRC) $(HEADER) tightsketch_simd.hpp
	g++ $(CFLAGS) -mavx2 -o $@ $< $(SRC) tightsketch_simd.cpp $(LIBS) 

clean:
	rm -rf $(EXEC)
	rm -rf *log*
	rm -rf *out*
