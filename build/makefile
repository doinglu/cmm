UNAME := $(shell uname)

ifeq ($(UNAME),Linux)
MACRO =
LINK_PLATFORM = -lrt
endif

ARCH = -m64
CFLAGS = -O2 $(ARCH) -I../std/include
CPPFLAGS = $(CFLAGS) -std=c++11 
LFLAGS = $(ARCH) -lstdc++ -pthread $(LINK_PLATFORM)

OBJS := mts/mts.o


STD_SRCS := $(notdir $(shell ls ../std/src/std_port/*.c))
STD_OBJS := $(addprefix std/src/std_port/, $(addsuffix .o, $(basename $(STD_SRCS)))) std/src/std_memmgr/std_memmgr.o

a.out : $(OBJS) $(STD_OBJS)
	@echo $(STD_SRCS)
	@echo $(STD_OBJS)
	g++ -o $@ $(OBJS) $(STD_OBJS) $(LFLAGS)

VPATH = $(SRCDIR)

SRCDIR = ../

clean :
	rm -rf mts std


.c.o :
	+@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	gcc $(CFLAGS) -o $@ -c $<

.cpp.o :
	+@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	g++ $(CPPFLAGS) -o $@ -c $<

