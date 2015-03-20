CXXFLAGS += -std=c++11 -Wall -O2 -I../src -static
all: chat

chat: chat.o ../src/libhttp_msg.a
	    $(CXX) -o $@ $^ -lcurl -L ../src -lhttp_msg

clean:
	rm chat *.o
