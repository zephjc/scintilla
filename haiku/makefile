# Make file for Scintilla on Haiku
# Copyright 1998-2010 by Neil Hodgson <neilh@scintilla.org>
# Copyright 2011 by Andrea Anzani <andrea.anzani@gmail.com>
# Copyright 2014-2015 by Kacper Kasper <kacperkasper@gmail.com>
# The License.txt file describes the conditions under which this software may be distributed.
# This makefile assumes GCC 4.7 is used and changes will be needed to use other compilers.

.SUFFIXES: .cxx .c .o .h .a
ifdef CLANG
CC = clang
CCOMP = clang
else
CC = g++
CCOMP = gcc
endif
AR = ar
RANLIB = touch

COMPLIB=../bin/scintilla.a

vpath %.h ../src ../include ../lexlib
vpath %.cxx ../src ../lexlib ../lexers

INCLUDEDIRS=-I ../include -I ../src -I ../lexlib
CXXBASEFLAGS=-Wall -Wno-missing-braces -Wno-char-subscripts -Wno-long-long -pedantic -DHAIKU -DSCI_NAMESPACE -DSCI_LEXER $(INCLUDEDIRS) $(DEPRECATED)

ifdef NOTHREADS
THREADFLAGS=-DG_THREADS_IMPL_NONE
else
THREADFLAGS=
endif

ifdef DEBUG
CXXFLAGS=-DDEBUG -g $(CXXBASEFLAGS) $(THREADFLAGS)
else
CXXFLAGS=-DNDEBUG -Os $(CXXBASEFLAGS) $(THREADFLAGS)
endif

.cxx.o:
	$(CC) -std=c++11 $(CONFIGFLAGS) $(CXXFLAGS) -c $<
.c.o:
	$(CCOMP) $(CONFIGFLAGS) $(CXXFLAGS) -w -c $<

LEXOBJS:=$(addsuffix .o,$(basename $(notdir $(wildcard ../lexers/Lex*.cxx))))

all: $(COMPLIB)

clean:
	rm -f *.o $(COMPLIB)

deps:
	$(CC) -MM $(CONFIGFLAGS) $(CXXFLAGS) *.cxx ../src/*.cxx | sed -e 's/\/usr.* //' | grep [a-zA-Z] >deps.mak

$(COMPLIB): Accessor.o CharacterSet.o LexerBase.o LexerModule.o LexerSimple.o \
	StyleContext.o WordList.o EditModel.o EditView.o MarginView.o CharClassify.o \
	Decoration.o Document.o PerLine.o Catalogue.o CallTip.o ScintillaBase.o \
	CharacterCategory.o CaseFolder.o CaseConvert.o ContractionState.o Editor.o \
	ExternalLexer.o PropSetSimple.o PlatHaiku.o KeyMap.o LineMarker.o PositionCache.o \
	ScintillaHaiku.o CellBuffer.o ViewStyle.o RESearch.o RunStyles.o Selection.o \
	Style.o Indicator.o AutoComplete.o UniConversion.o XPM.o \
	$(LEXOBJS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

# Automatically generate header dependencies with "make deps"
include deps.mak
