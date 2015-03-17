#include "chic_comm.h"
#include <ctime>
#include <iostream>
#include <exception>


using namespace chic;

int main(int argc, const char* argv[]) 
{
    Comm com;

	bool do_input = true;
	if (argc >= 3) {
	   if (strcmp("-in", argv[1]) == 0)
		   com.register_input_channel("in", argv[2]);
	   else if (strcmp("-out", argv[1]) == 0) {
		   do_input = false;
		   com.register_output_channel("out", argv[2]);
	   }
	   else {
		   printf("You need to provide a -in or -out option\n");
		   return 1;
	   }
	}
	else {
		printf("You need to provide a -in or -out option\n");
		return 1;
	}
	if (do_input) {
		try {
		InChannel in = com.get_input_channel("in");
		while (true) {
			Message inmsg = in.try_take(3000); // wait for 3 seconds
			if (inmsg.isValid()) {
				std::string s(inmsg.payload().data(), inmsg.payload().size());
				std::cout << "[Msg:" << inmsg.id() << "]: "<< s << std::endl;
			}
			else
				std::cout << "----- no response ----" << std::endl;
		}
		}
		catch (const std::exception& e) {
			std::cout << "Exception: " << e.what() << std::endl;

		}
	}
	else {
		OutChannel out = com.get_output_channel("out");
		std::string line;
		std::cout << "> ";
		while (std::getline(std::cin, line)) {
			/* std::time_t result = std::time(NULL);
			std::string msg = std::ctime(&result);
			msg +="]: ";
			msg += line;
			out.put(msg);
			*/
			out.put(line);
			std::cout << "> ";
		}
	}

	/*
	InChannel in = com.get_input_channel("in");
	for (int i=0; i<300000; ++i) {
		Message inmsg = in.take();
		string s(inmsg.payload().data(), inmsg.payload().size());
		cout << "Got :" << s << endl;
	}
	*/
	return 0;

}

