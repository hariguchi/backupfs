CC        := gcc

OBJDIR := obj/
DEPDIR := dep/

ifeq ($(OS), Windows_NT)
PROF      :=
else
PROF      := #-pg
endif
LDLIBS    :=
OPTFLAGS  := -g 
DEFS      += 

TARGET    := libstringrbt.a
TSTTARGET := rbt-test

CXXFLAGS  := -std=c++11 -Wall -I/usr/include/boost $(PROF) $(OPTFLAGS) $(DEFS)
LOADLIBES := 
CSRCS     := $(TSTTARGET).c
CXXSRCS   := string-rbt.cc
OBJS      := $(addprefix $(OBJDIR),$(CXXSRCS:.cc=.o))
LIBOBJS   := $(addprefix $(TARGET),($(OBJS)))
EXAMPLES  := example1 example2 example3 example4 example5


$(TARGET): $(LIBOBJS)

$(TSTTARGET): $(OBJDIR)$(TSTTARGET).o $(OBJS)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

test:
	if [ ! -f $(TSTTARGET).c ]; then ln -s string-rbt.c.TEST $(TSTTARGET).c; fi
	$(MAKE) $(TSTTARGET)


include $(addprefix $(DEPDIR),$(CSRCS:.c=.d))
include $(addprefix $(DEPDIR),$(CXXSRCS:.cc=.d))


$(DEPDIR)%.d: %.c
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $(CFLAGS) $< | sed "s@$*.o@& $@@g " > $@'
$(DEPDIR)%.d: %.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $(CFLAGS) $< | sed "s@$*.o@& $@@g " > $@'

$(OBJDIR)%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(OBJDIR)%.o: %.cc
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

clean:
	rm -f $(TARGET) $(TSTTARGET) $(EXAMPLES) $(OBJDIR)*.o $(DEPDIR)*.d *.bak *~
