NAME	=	game
CXX		=	g++
DEBUG		+=	-Wall -g
CXXFLAGS	+=	-std=c++17 -O2 $(DEBUG)
LDLIBS = -l hiredis

all:  $(NAME)
	@echo "Building..."

tabuleiro.o: tabuleiro.c tabuleiro.h
	gcc -c tabuleiro.c

$(NAME): tabuleiro.o $(NAME).cpp
	$(CXX)  $^ $(CXXFLAGS) $(LDLIBS) -o $@

clean :	
	rm -rf $(NAME)
