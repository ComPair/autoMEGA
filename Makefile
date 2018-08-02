CC=g++

MAIN_FLAGS=-std=c++11 -pthread -lyaml-cpp -O2 -Wall
MEGALIB_FLAGS=`root-config --cflags --libs` -I$(MEGALIB)/include -L$(MEGALIB)/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc

all: clean checkGeometry autoMEGA

noMEGAlib: clean autoMEGA

debug: clean checkGeometry debug-autoMEGA

debug-noMEGAlib: clean debug-autoMEGA

checkGeometry:
		$(CC) checkGeometry.cpp -o checkGeometry $(MAIN_FLAGS) $(MEGALIB_FLAGS)

autoMEGA:
		$(CC) autoMEGA.cpp -o autoMEGA $(MAIN_FLAGS)

debug-autoMEGA:
		git submodule add https://github.com/bombela/backward-cpp
		git submodule update --init --remote --recursive
		$(CC) autoMEGA.cpp -o autoMEGA $(MAIN_FLAGS) -D DEBUG

clean:
		rm -f autoMEGA checkGeometry
