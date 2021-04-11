# Makefile for a version number updater inherited from SoftLab, thanks Reibert Arbring!

CFLAGS	= $(DEBUG) $(CQ)
LDFLAGS = $(LQ)
DEBUG	= -g
CC	= gcc -ansi
#CQ	= -DSPA_LANG=46

#-- Product info --
NAME = venum
SRCS = vn.c vnw.c spa.c


ifeq ($(findstring CYGWIN, $(shell uname)), CYGWIN)
  ifeq ($(shell uname -m), i686)
	OS=cygwin32
  else
	OS=cygwin64
  endif
else
  ifeq ($(findstring MINGW64, $(shell uname)), MINGW64)
	OS=mingw64
  else
	OS=$(shell uname)
  endif
endif


BINDIR = /usr/local/bin
MANDIR = /usr/local/man

PROG = ../bin/$(OS)/$(NAME)

#-- C rules for building $(OS)/% --

../bin/$(OS)/%: %.c
	$(LINK.c) -o $@ $< $(LDLIBS)
../bin/$(OS)/%.o: %.c
	$(COMPILE.c) -D__$(OS)__ $(OUTPUT_OPTION) -o $@ $<
../bin/$(OS)/%a: %.c
	$(COMPILE.c) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

OBJS =  ${SRCS:%.c=../bin/$(OS)/%.o}


#--
product debug: ../bin/$(OS) $(PROG)

debug:=			DEBUG= -g -DDBG
installdebug:=  DEBUG= -g -DDBG
install:=       DEBUG= -O
all:=           DEBUG= -O

$(PROG):  $(OBJS) $(NAME).version.c
	$(CC) -o $@ $(CQ) $(OBJS) $(NAME).version.c $(LDFLAGS)

# Use venum to generate version number if available
$(NAME).version.c: $(OBJS)
ifneq ($(shell which venum 2>/dev/null), )
	-venum $(NAME) time
else
	@echo "venum not available (yet), keeping old timestamps"
endif

$(NAME).version.h + version.h:
ifneq ($(shell which venum 2>/dev/null), )
	venum -all -write always $(NAME)
else
	@echo "venum not available (yet), keeping old timestamps"
endif

#--
install installdebug: $(PROG) $(MANDIR)/man1/$(NAME).1
	install -c -d -m 755 $(BINDIR)
	install -c -s -m 755 $(PROG) $(BINDIR)

$(MANDIR)/man1/$(NAME).1: ../man/$(NAME).1
	install -c -m 444 ../man/$(NAME).1 $(MANDIR)/man1

#--
clean:
	-$(RM) *~ $(OBJS) core

veryclean: clean
	-$(RM) $(PROG) $(NAME).version.c $(NAME).version.h version.h
	-rm -rf ../bin/$(OS)

#--
../bin/$(OS):
	@-mkdir -p ../bin/$(OS) 2> /dev/null

# EoF
