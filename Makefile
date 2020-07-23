#
# Lick Makefile
#


# ------------------------------------------
#  Variables
#

BINDIR			= bin/
SRCDIR			= src/
LICKDIR			= $(SRCDIR)lick/
FLICKDIR		= $(SRCDIR)flick/
LIBDIR			= $(SRCDIR)libs/
TESTSDIR		= $(SRCDIR)drivers/

LICKNAME		= $(BINDIR)lick
FLICKNAME		= $(BINDIR)flick
TESTNAME		= $(BINDIR)testlibs
BWTRANDNAME = $(BINDIR)randbwt
BITQNAME		= $(BINDIR)bitqtest

INCLUDEPATH		= -I$(LIBDIR) -I$(LIBDIR)gnu/

DEFINES = -DUNIX -DUFS_COMP
 
CC          = gcc
CCNOWARN		= -Wno-unused-variable -Wno-unused-parameter
CCWARN			= -W -Wall $(CCNOWARN) -Wformat -Wformat-security -Wstack-protector

# gcover support
#CCDEBUG			= -ggdb -fprofile-arcs -ftest-coverage

# just the debug symbols
CCDEBUG			= -ggdb -O0

CCFLAGS     = -c $(CCWARN) $(CCDEBUG) $(DEFINES) $(INCLUDEPATH) -std=c99 -pedantic

LINKER	    = gcc
#LINKFLAGS   = -lefence


# ------------------------------------------
#   Default rules
#
.c.o:
	@echo "  CC     $*.c"
	@$(CC) $(CCFLAGS) $*.c -o $*.o

# ------------------------------------------
# Makefile dependencies
#

ALL: $(LICKNAME) $(FLICKNAME) $(TESTNAME) $(BWTRANDNAME) $(BITQNAME)

LICKOBJS = $(LICKDIR)lick.o $(LICKDIR)add.o $(LICKDIR)fileformat.o $(LICKDIR)platform.o $(LIBDIR)crc32_lib.o $(LIBDIR)llist_lib.o $(LIBDIR)bitq_lib.o $(LIBDIR)bwt_lib.o $(LIBDIR)mtf_lib.o $(LIBDIR)rle_lib.o $(LIBDIR)huff_lib.o $(LIBDIR)compress_lib.o
FLICKOBJS = $(FLICKDIR)flick.o $(LIBDIR)bitq_lib.o $(LIBDIR)bwt_lib.o $(LIBDIR)mtf_lib.o $(LIBDIR)rle_lib.o $(LIBDIR)huff_lib.o $(LIBDIR)compress_lib.o
TESTOBJS = $(TESTSDIR)testlibs.o $(LIBDIR)bitq_lib.o $(LIBDIR)bwt_lib.o $(LIBDIR)mtf_lib.o $(LIBDIR)rle_lib.o $(LIBDIR)huff_lib.o
BWTRANDOBJS = $(TESTSDIR)randbwt.o $(LIBDIR)bwt_lib.o
BITQOBJS = $(TESTSDIR)bitqtest.o $(LIBDIR)bitq_lib.o

$(LICKNAME): $(LICKOBJS)
	@mkdir -p $(BINDIR)
	@echo "  LD     $(LICKNAME)"
	@$(LINKER) $(LINKFLAGS) $(LICKOBJS) -o $(LICKNAME)

$(FLICKNAME): $(FLICKOBJS)
	@mkdir -p $(BINDIR)
	@echo "  LD     $(FLICKNAME)"
	@$(LINKER) $(LINKFLAGS) $(FLICKOBJS) -o $(FLICKNAME)

$(TESTNAME): $(TESTOBJS)
	@mkdir -p $(BINDIR)
	@echo "  LD     $(TESTNAME)"
	@$(LINKER) $(LINKFLAGS) $(TESTOBJS) -o $(TESTNAME)

$(BWTRANDNAME): $(BWTRANDOBJS)
	@mkdir -p $(BINDIR)
	@echo "  LD     $(BWTRANDNAME)"
	@$(LINKER) $(LINKFLAGS) $(BWTRANDOBJS) -o $(BWTRANDNAME)

$(BITQNAME): $(BITQOBJS)
	@mkdir -p $(BINDIR)
	@echo "  LD     $(BITQNAME)"
	@$(LINKER) $(LINKFLAGS) $(BITQOBJS) -o $(BITQNAME)

$(LICKDIR)lick.o:				$(LICKDIR)lick.c $(LICKDIR)lick.h $(LICKDIR)add.h $(LICKDIR)locale.h $(LIBDIR)llist_lib.h
$(LICKDIR)add.o:				$(LICKDIR)add.c $(LICKDIR)lick.h $(LICKDIR)locale.h $(LICKDIR)fileformat.h $(LICKDIR)platform.h $(LIBDIR)crc32_lib.h $(LIBDIR)llist_lib.h
$(LICKDIR)fileformat.o:	$(LICKDIR)fileformat.c $(LICKDIR)fileformat.h $(LICKDIR)locale.h
$(LICKDIR)platform.o:		$(LICKDIR)platform.c $(LICKDIR)platform.h

$(FLICKDIR)flick.o:			$(LIBDIR)compress_lib.h

$(LIBDIR)crc32_lib.o:		$(LIBDIR)crc32_lib.c $(LIBDIR)crc32_lib.h
$(LIBDIR)bitq_lib.o:		$(LIBDIR)bitq_lib.c
$(LIBDIR)bwt_lib.o:			$(LIBDIR)bwt_lib.c $(LIBDIR)bwt_lib.h
$(LIBDIR)mtf_lib.o:			$(LIBDIR)mtf_lib.c $(LIBDIR)mtf_lib.h
$(LIBDIR)rle_lib.o:			$(LIBDIR)rle_lib.c $(LIBDIR)rle_lib.h
$(LIBDIR)huff_lib.o:		$(LIBDIR)huff_lib.c $(LIBDIR)huff_lib.h $(LIBDIR)bitq_lib.h
$(LIBDIR)llist_lib.o:		$(LIBDIR)llist_lib.c $(LIBDIR)llist_lib.h
$(LIBDIR)compress_lib.o:	$(LIBDIR)compress_lib.c $(LIBDIR)bwt_lib.h  $(LIBDIR)mtf_lib.h  $(LIBDIR)rle_lib.h  $(LIBDIR)huff_lib.h 

$(LIBDIR)qsmodel.o:			$(LIBDIR)qsmodel.c $(LIBDIR)qsmodel.h

$(TESTSDIR)testlibs.o:	$(TESTSDIR)testlibs.c $(LIBDIR)rle_lib.h $(LIBDIR)mtf_lib.h $(LIBDIR)bwt_lib.h $(LIBDIR)huff_lib.h 
$(TESTSDIR)randbwt.o:		$(TESTSDIR)randbwt.c $(LIBDIR)bwt_lib.h
$(TESTSDIR)bitqtest.o:	$(TESTSDIR)bitqtest.c $(LIBDIR)bitq_lib.h


clean:
	rm -f $(BINDIR)* $(LIBDIR)*.o $(LICKDIR)*.o $(FLICKDIR)*.o $(TESTSDIR)*.o

touch:
	touch $(LIBDIR)*.c $(LICKDIR).c $(TESTSDIR)*.c
