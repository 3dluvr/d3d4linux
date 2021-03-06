
BINARIES = compile-hlsl d3d4linux.exe

INCLUDE = include/d3d4linux.h \
          include/d3d4linux_common.h \
          include/d3d4linux_enums.h \
          include/d3d4linux_impl.h \
          include/d3d4linux_types.h

CXXFLAGS += -Wall -I./include -std=c++11

ifeq ($(OS), Windows_NT)
CXX := x86_64-w64-mingw32-c++
LDFLAGS = -s -static-libgcc -static-libstdc++ -ldxguid -static -ld3dcompiler -static -lpthread
else
LDFLAGS = -g
endif

all: $(BINARIES)

d3d4linux.exe: d3d4linux.cpp $(INCLUDE) Makefile
	x86_64-w64-mingw32-c++ $(CXXFLAGS) $(filter %.cpp, $^) -static -o $@ -ldxguid

compile-hlsl: compile-hlsl.cpp $(INCLUDE) Makefile
	$(CXX) $(CXXFLAGS) $(filter %.cpp, $^) -o $@ $(LDFLAGS)

check: all
	D3D4LINUX_VERBOSE=1 ./compile-hlsl sample_ps.hlsl ps_main ps_4_0

clean:
	rm -f $(BINARIES)

