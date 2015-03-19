#include "chic_comm.h"
#include <ctime>
#include <iostream>
#include <exception>


using namespace chic;

int main(int argc, const char* argv[]) 
{
    Comm com;
    com.init(argc, argv);

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
                /*
                Message msg;
                bool gotit = in.try_get(3000, msg); // wait for 3 seconds
                if (gotit) {
                    std::string s(msg.payload().data(), msg.payload().size());
                    std::cout << "[Msg:" << msg.id() << "]: "<< s << std::endl;
                }
                else
                    std::cout << "----- no response ----" << std::endl;
                    */
                Message msg = in.get();
                std::string s(msg.payload().data(), msg.payload().size());
                std::cout << "[Msg:" << msg.id() << "]: "<< s << std::endl;
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
			out.put(line);
			std::cout << "> ";
		}
	}

	return 0;

}

