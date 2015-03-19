CXXFLAGS += -std=c++11 -Wall -O2
all: chic_com

chic_com: test.o chic_comm.o jsonxx.o
	$(CXX) -o $@ $^ -lcurl
