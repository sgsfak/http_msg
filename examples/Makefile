CXXFLAGS += -std=c++11 -Wall -g -I../src -static
all: chat os bm

chat: chat.o ../src/libhttp_msg.a
	    $(CXX) -o $@ $^ -lcurl -L ../src -lhttp_msg

os: oncosim.o ../src/libhttp_msg.a
	    $(CXX) -o $@ $^ -lcurl -L ../src -lhttp_msg

bm: biomechanics.o ../src/libhttp_msg.a
	    $(CXX) -o $@ $^ -lcurl -L ../src -lhttp_msg

clean:
	rm chat os bm *.o
