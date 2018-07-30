CC=g++

MAIN_FLAGS=-std=c++11 -lX11 -lXtst -pthread -ldl -ldw -lyaml-cpp -g -lcurl -Ofast -Wall
MEGALIB_FLAGS=`root-config --cflags --libs` -I$(MEGALIB)/include -L$(MEGALIB)/lib -lGeomegaGui -lGeomega -lCommonGui -lCommonMisc

all: clean init pipeline checkGeometry autoMEGA

noMEGAlib: clean init pipeline autoMEGA

init:
		git submodule update --init --recursive --remote

pipeline:
		cd pipeliningTools; $(CC) -c pipeline.h $(MAIN_FLAGS)

checkGeometry:
		$(CC) checkGeometry.cpp -o checkGeometry $(MAIN_FLAGS) $(MEGALIB_FLAGS)

autoMEGA:
		$(CC) autoMEGA.cpp -o autoMEGA $(MAIN_FLAGS)

clean:
		rm -f autoMEGA checkGeometry *.legend *.out
