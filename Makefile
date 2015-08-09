CC=gcc
CXX=g++
CXXFLAGS=-Wall -O3
DEFINES=
INCLUDES=
THIRDLIB_DIR=
LIBS=

TARGET=
#Only .cpp files in present working directory.
SRCS=$(wildcard *.cpp)

#PWD/.cpp files and its sub-directory .cpp files.
#DIRS=$(shell find . -maxdepth 10 -type d)
#SRC_CPP=$(foreach dir,$(DIRS),$(wildcard $(dir)/*.cpp))
#SRC_C=$(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))

OBJDIR=./objs
OBJS=$(patsubst %.cpp,$(OBJDIR)/%.o,$(SRCS))
DEPS=$(patsubst %.cpp,$(OBJDIR)/%.d,$(SRCS))

all:$(TARGET)

$(OBJDIR)/%.o:%.cpp
	@mkdir -p $(@D)
#-MMD means create rely files. -Mf means change .d files default name. -MT means change $@ compile ways.
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MF $(@:.o=.d) -MT $@ -c $< -o $@

-include $(DEPS)

$(TARGET):$(OBJS)
	$(CXX) -o $@ $(OBJS) $(THIRDLIB_DIR) $(LIBS)

clean:
	rm -rf $(DEPS) $(OBJS) $(OBJDIR) $(TARGET)
