CXXFLAGS += -std=c++11 -Wall -g
all: chic_com

chic_com: test.o chic_comm.o happyhttp.o jsonxx.o
	$(CXX) -o $@ $^
