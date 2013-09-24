CXX := g++
LLVMCOMPONENTS := cppbackend
RTTIFLAG := -fno-rtti
LLVMCONFIG := /usr/lib/llvm-3.4/bin/llvm-config

CXXFLAGS := -O0 -ggdb  -std=c++11 -I$(shell $(LLVMCONFIG) --src-root)/tools/clang/include -I$(shell $(LLVMCONFIG) --obj-root)/tools/clang/include $(shell $(LLVMCONFIG) --cxxflags) $(RTTIFLAG)
LLVMLDFLAGS := $(shell $(LLVMCONFIG) --ldflags --libs $(LLVMCOMPONENTS))

SOURCES = CPPPlantGenerator.cpp 

OBJECTS = $(SOURCES:.cpp=.o)

EXES = $(OBJECTS:.o=)

CLANGLIBS = \
-lclang \
-lboost_regex 

all: $(OBJECTS) $(EXES)

%: %.o
		$(CXX) -ggdb  -o $@ $< $(CLANGLIBS) $(LLVMLDFLAGS)

clean:
		-rm -f $(EXES) $(OBJECTS) *~