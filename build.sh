rm build 2>/dev/null

CPP_FLAGS="-Wall -g -O3"
LIBS="-lsfml-graphics -lsfml-window -lsfml-system -pthread"

g++ -c $CPP_FLAGS main.cpp 
test -f main.o && g++ $CPP_FLAGS *.o -o game_of_life $LIBS 

rm *.o 2>/dev/null
test -f game_of_life && ./game_of_life
