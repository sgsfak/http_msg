CXXFLAGS += -std=c++11 -Wall -O2
all: chic_com

libhttp_msg.a: chic_comm.o jsonxx.o
	ar -rcs $@ $^
chic_com: test.o libhttp_msg.a
	$(CXX) -o $@ $< -L. -lhttp_msg -lcurl

