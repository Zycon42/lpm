# Projekt k predmetu PDS/2013L
# autor: Jan Dusek <xdusek17@stud.fit.vutbr.cz>
#
# Makefile
#

NAME=lpm

CXX=g++
CXXFLAGS=-Wall -pedantic -O2

ifeq ($(OS), Windows_NT)
    LDFLAGS=-lws2_32 -static-libgcc -static-libstdc++
else
    LDFLAGS=
endif

OBJFILES=main.o

.PHONY: all clean dep

all: $(NAME)

-include includes.dep

$(NAME): $(OBJFILES)
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:
	rm -f *.o $(NAME)

dep:
	$(CXX) -MM *.cpp > includes.dep
