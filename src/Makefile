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
OSTYPE=cygwin32
else
OSTYPE=cygwin64
endif
endif

BINDIR = ../bin/$(OSTYPE)
MANDIR = ../man

PROG = $(OSTYPE)/$(NAME)

#-- C rules for building $(OSTYPE)/% --

$(OSTYPE)/%: %.c
	$(LINK.c) -o $@ $< $(LDLIBS)
$(OSTYPE)/%.o:  %.c
	$(COMPILE.c) -D__$(OSTYPE)__ $(OUTPUT_OPTION) -o $@ $<
$(OSTYPE)/%a: %.c
	$(COMPILE.c) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

OBJS =  ${SRCS:%.c=$(OSTYPE)/%.o}


#--
product debug: $(PROG)

debug:=		DEBUG= -g -DDBG
installdebug:=  DEBUG= -g -DDBG
install:=       DEBUG= -O
all:=   DEBUG= -O

$(PROG):  $(OBJS) $(NAME).version.c
	$(CC) -o $@ $(CQ) $(OBJS) $(NAME).version.c $(LDFLAGS)

$(NAME).version.c: $(OBJS)
	-venum $(NAME) time

$(NAME).version.h + version.h:
	venum -all -write always $(NAME)

#--
install installdebug: $(PROG) $(MANDIR)/man1/$(NAME).1
	[ $(OSTYPE) = HP-UX ] && cp $(PROG) $(BINDIR) || install -c -s -m 755 $(PROG) $(BINDIR)

$(MANDIR)/man1/$(NAME).1: ../man/$(NAME).1
	install -c -m 444 ../man/$(NAME).1 $(MANDIR)/man1

#--
clean:
	-$(RM) *~ $(OBJS) core

veryclean: clean
	-$(RM) $(PROG) $(NAME).version.c $(NAME).version.h version.h
	-rmdir $(OSTYPE)

#--
.INIT::
	@-mkdir $(OSTYPE) 2> /dev/null

# EoF
