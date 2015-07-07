# $Id: Makefile,v 1.27 2015/04/10 21:44:13 cvsremote Exp $
#
#   Copyright (c) 2015, Yoichi Hariguchi
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#       o Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#       o Redistributions in binary form must reproduce the above
#         copyright notice, this list of conditions and the following
#         disclaimer in the documentation and/or other materials provided
#         with the distribution.
#       o Neither the name of the Yoichi Hariguchi nor the names of its
#         contributors may be used to endorse or promote products derived
#         from this software without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

CC        := gcc
TARGET    := backupfs

OBJDIR := obj/
DEPDIR := dep/

#
# Red-black tree library
#
RBT    := string-rbt
RBTDIR := $(RBT)/
RBTLIB := $(RBTDIR)libstringrbt.a

ifeq ($(OS), Windows_NT)

DEFS       += -DOS_WINDOWS_NT
PROF       :=
GETLINESRC := getline.c
OWNER      := Administrator
GROUP      := Administrators
TGTGRP     := $(GROUP)
EXE        := .exe

else

DEFS       += -DOS_UNIX
PROF       := #-pg
GETLINESRC :=
OWNER      := root
GROUP      := root
TGTGRP     := $(TARGET)
EXE        :=

endif

LDLIBS    :=
OPTFLAGS  := -g 

CFLAGS      := -Wall -I$(RBT) $(PROF) $(OPTFLAGS) $(DEFS)
LDFLAGS     :=
LOADLIBES   := 

ALLTARGETS  := $(TARGET)-all
RMTTARGET   := $(TARGET)-remote
CHKSRCTGT   := $(TARGET)-chksrc
EXECTARTGT  := $(TARGET)-exctar
MKDIRTARTGT := $(TARGET)-mkdir
MKLNKTARTGT := $(TARGET)-mklink
SHELLTGT    := $(TARGET)-shell
HISTTGT     := $(TARGET)-hist
CHGFILETGT  := changedfiles
NEWFILETGT  := newfiles
ALL_TARGETS := $(TARGET) $(RMTTARGET) $(CHKSRCTGT) $(EXECTARTGT) \
			   $(MKDIRTARTGT) $(MKLNKTARTGT) $(SHELLTGT) $(HISTTGT) \
	           $(NEWFILETGT)
LOCALSRCS   := backupfs-local.c main-local.c
RMTSRCS     := $(RMTTARGET).c main-remote.c
CHKSRCSRCS  := $(CHKSRCTGT).c error.c
EXECTARSRCS := $(EXECTARTGT).c error.c
MKDIRSRCS   := $(MKDIRTARTGT).c error.c $(GETLINESRC)
MKLNKSRCS   := $(MKLNKTARTGT).c error.c $(GETLINESRC)
SHELLSRCS   := $(SHELLTGT).c
HISTSRCS    := $(HISTTGT).c error.c
NEWFILESRCS := $(NEWFILETGT).c dirwalk.c error.c file.c $(GETLINESRC)
CMMNSRCS    := backupfs.c dirwalk.c file.c error.c date.c $(GETLINESRC)
SRCS        := $(wildcard *.c)
LOCALOBJS   := $(addprefix $(OBJDIR),$(LOCALSRCS:.c=.o))
RMTOBJS     := $(addprefix $(OBJDIR),$(RMTSRCS:.c=.o))
CHKSRCOBJS  := $(addprefix $(OBJDIR),$(CHKSRCSRCS:.c=.o))
EXECTAROBJS := $(addprefix $(OBJDIR),$(EXECTARSRCS:.c=.o))
MKDIROBJS   := $(addprefix $(OBJDIR),$(MKDIRSRCS:.c=.o))
MKLNKOBJS   := $(addprefix $(OBJDIR),$(MKLNKSRCS:.c=.o))
SHELLOBJS   := $(addprefix $(OBJDIR),$(SHELLSRCS:.c=.o))
HISTOBJS    := $(addprefix $(OBJDIR),$(HISTSRCS:.c=.o))
NEWFILEOBJS := $(addprefix $(OBJDIR),$(NEWFILESRCS:.c=.o))
CMMNOBJS    := $(addprefix $(OBJDIR),$(CMMNSRCS:.c=.o))
#LIBOBJS     := $(addprefix $(OBJDIR)$(TARGET),($(OBJS)))

TGTUID      := 65530
TGTGID      := 65530
PREFIX      := /usr/local
BINDIR      := $(PREFIX)/bin
MANDIR      := $(PREFIX)/man
BKUPFSHOME  := /home/backupfs


$(ALLTARGETS): $(ALL_TARGETS)
$(TARGET) : $(LOCALOBJS) $(CMMNOBJS) $(RBTLIB)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(RMTTARGET) : $(RMTOBJS) $(CMMNOBJS) $(RBTLIB)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(CHKSRCTGT) : $(CHKSRCOBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(EXECTARTGT) : $(EXECTAROBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(MKDIRTARTGT) : $(MKDIROBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(MKLNKTARTGT) : $(MKLNKOBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(SHELLTGT) : $(SHELLOBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(HISTTGT) : $(HISTOBJS) $(OBJDIR)date.o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(NEWFILETGT) : $(NEWFILEOBJS) $(OBJDIR)date.o $(RBTLIB)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

$(RBTLIB):
	cd $(RBT) && $(MAKE)

date.c: $(filter-out date.c, $(SRCS)) $(wildcard *.h)
	echo "char CompilationDate[] = " > date.c
	echo '"'`date`'";' >> date.c



include $(addprefix $(DEPDIR),$(SRCS:.c=.d))

$(DEPDIR)%.d: %.c
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $(CFLAGS) $< | sed "s@$*.o@& $@@g " > $@'

$(OBJDIR)%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(OBJDIR)%.o: %.cc
	$(COMPILE.cc) $(OUTPUT_OPTION) $<




install: install-bin install-man

install-bin: $(ALLTARGETS)
ifneq ($(OS), Windows_NT)
	egrep $(TARGET) /etc/passwd >/dev/null || \
	  adduser --uid $(TGTUID) --home $(BKUPFSHOME) \
	  --shell $(BINDIR)/$(SHELLTGT) --disabled-password \
	  --gecos 'backupfs, daily backup system' $(TARGET)
	egrep $(TARGET) /etc/group >/dev/null || groupadd -g $(TGTGID) $(TARGET)
	mkdir -p $(BKUPFSHOME)/.ssh && \
	chmod 700 $(BKUPFSHOME)/.ssh && \
	touch $(BKUPFSHOME)/.ssh/authorized_keys && \
	chown -R $(TGTUID):$(TGTGID) $(BKUPFSHOME)/.ssh
endif
	install -c -m 555 -o $(OWNER) -g $(GROUP) \
	  $(TARGET) $(MKDIRTARTGT) $(SHELLTGT) $(HISTTGT) $(MKLNKTARTGT) \
	  $(NEWFILETGT) $(BINDIR)
	install -c -m 4555 -o $(OWNER) -g $(TGTGRP) \
	  $(RMTTARGET) $(CHKSRCTGT) $(EXECTARTGT) $(BINDIR)
	(cd $(BINDIR); \
	 if [ -e $(CHGFILETGT) ]; then \
	   rm $(CHGFILETGT)$(EXE) ; \
	 fi ; \
	 ln -s $(NEWFILETGT)$(EXE) $(CHGFILETGT)$(EXE) ; \
	)

install-man:
	if [ ! -d $(MANDIR) ]; then mkdir $(MANDIR); fi
	if [ ! -d $(MANDIR)/man1 ]; then mkdir $(MANDIR)/man1; fi
	if [ ! -d $(MANDIR)/man8 ]; then mkdir $(MANDIR)/man8; fi
	gzip < $(HISTTGT).man > $(MANDIR)/man1/$(HISTTGT).1.gz
	gzip < $(TARGET).man > $(MANDIR)/man8/$(TARGET).8.gz
	gzip < $(NEWFILETGT).man > $(MANDIR)/man8/$(NEWFILETGT).8.gz
	gzip < $(CHGFILETGT).man > $(MANDIR)/man8/$(CHGFILETGT).8.gz

ssh-keygen:
	ssh-keygen -t rsa -f id_rsa -N ''
	if [ ! -d $(BKUPFSHOME) ]; then \
		mkdir $(BKUPFSHOME)/.ssh ;\
		chown backupfs.backupfs $(BKUPFSHOME)/.ssh ;\
	fi

clean:
	rm -f $(ALL_TARGETS) $(OBJDIR)*.o $(DEPDIR)*.d *.bak *~
	cd string-rbt && $(MAKE) clean


# mkdir /home/backupfs/.ssh
# chmod 700 /home/backupfs/.ssh
# chown backupfs.backupfs /home/backupfs/.ssh
# cp id_rsa /home/backupfs/.ssh/.id_rsa
# chown backupfs.backupfs /home/backupfs/.ssh/.id_rsa

