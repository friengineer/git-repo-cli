# HCI makefile
#
# GNU Make has most of the rules built in.
# https://www.gnu.org/software/make/manual/html_node/Catalogue-of-Rules.html
# no reason to copy them.

CPPFLAGS=-DTRACE_UNTESTED -std=c++11
CXXFLAGS=-std=c++11 -Wall -pedantic

GIT_HCI_PROGRAMS = main
HCI_H = hci0.h
GITPP_H = gitpp5.h

HCI_PROGRAMS = ${GIT_HCI_PROGRAMS}
GIT_PROGRAMS = ${GIT_HCI_PROGRAMS}

all: ${HCI_PROGRAMS} ${GIT_PROGRAMS}

${HCI_PROGRAMS}: hci.o

hci.o: CXXFLAGS+=-fPIC
${HCI_PROGRAMS:%=%.o}: CXXFLAGS=-fPIC

${HCI_PROGRAMS}: LDLIBS=-lstdc++
${GIT_PROGRAMS}: LDLIBS=-lstdc++ -lgit2

${HCI_PROGRAMS:%=%.o}: ${HCI_H}
${GIT_PROGRAMS:%=%.o}: ${GITPP_H}

CLEANFILES = ${HCI_PROGRAMS}
clean:
	rm -f *~ *.o ${CLEANFILES}

.SUFFIXES:
.SUFFIXES: .o .cc
