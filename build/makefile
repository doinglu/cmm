UNAME := $(shell uname)

ifeq ($(UNAME),Linux)
CPPFLAGS_MACRO = -Wno-invalid-offsetof
LINK_PLATFORM = -lrt -pthread
endif

ARCH = -m64
CFLAGS = -O2 $(ARCH) -I../std/include
CPPFLAGS = $(CFLAGS) -std=c++11 $(CPPFLAGS_MACRO)
LFLAGS = $(ARCH) -lstdc++ $(LINK_PLATFORM)

SRCS := $(notdir $(shell ls ../mts/*.cpp))
OBJS := $(addprefix mts/, $(addsuffix .o, $(basename $(SRCS))))

STD_SRCS := $(notdir $(shell ls ../std/src/std_port/*.c))
STD_OBJS := $(addprefix std/src/std_port/, $(addsuffix .o, $(basename $(STD_SRCS)))) std/src/std_memmgr/std_memmgr.o std/src/std_template/simple_string.o

a.out : $(OBJS) $(STD_OBJS)
	@echo $(STD_SRCS)
	@echo $(STD_OBJS)
	@echo $(SRCS)
	@echo $(OBJS)
	g++ -o $@ $(OBJS) $(STD_OBJS) $(LFLAGS)

VPATH = $(SRCDIR)

SRCDIR = ../

clean :
	rm -rf a.out mts std


.c.o :
	+@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	gcc $(CFLAGS) -o $@ -c $<

.cpp.o :
	+@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	g++ $(CPPFLAGS) -o $@ -c $<

