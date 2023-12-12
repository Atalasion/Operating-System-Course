CXX = g++
CXXFLAGS = -std=c++17
LDFLAGS += -pthread
CCFLAGS += -l SDL2 -l SDL2_image -l SDL2_ttf -l SDL2_mixer $(LDFLAGS)


all: source.out main.out building.out bills.out

source.out: source.o csv.h log.o
	$(CXX) $(CXXFLAGS) log.o csv.h source.o $(CCFLAGS) -o source.out 
main.out: main.o log.o
	$(CXX) $(CXXFLAGS) log.o main.o $(CCFLAGS) -o main.out
building.out: building.o utils.o log.o 
	$(CXX) $(CXXFLAGS) log.o building.o utils.o $(CCFLAGS) -o building.out
bills.out: bills.o utils.o log.o
	$(CXX) $(CXXFLAGS) bills.o log.o utils.o $(CCFLAGS) -o bills.out
source.o: csv.h source.cpp log.hpp
	$(CXX) $(CXXFLAGS) -c source.cpp -o source.o
main.o: main.cpp log.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

building.o: utils.hpp building.cpp log.hpp
	$(CXX) $(CXXFLAGS) -c building.cpp -o building.o

bills.o: utils.hpp bills.cpp log.hpp
	$(CXX) $(CXXFLAGS) -c bills.cpp -o bills.o

utils.o: utils.hpp utils.cpp log.hpp
	$(CXX) $(CXXFLAGS) -c utils.cpp -o utils.o
log.o: log.hpp log.cpp
	$(CXX) $(CXXFLAGS) -c log.cpp -o log.o

.PHONY: clean
clean:
	rm -f *.o
