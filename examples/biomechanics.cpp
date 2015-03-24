#include <fstream>
#include <cmath>
#include <stdio.h>
#include <string>
#include <sstream>

#include "chic_comm.h"

using namespace std;
void compute_press_map(const string& filename, int tumor_radius)
{
    ofstream press_map(filename);

    for (int x=0;x< tumor_radius;x++){      
        for (int y=0;y< tumor_radius;y++){      
            for (int z=0;z< tumor_radius;z++){
                double f = 2.0 * M_PI * rand() / RAND_MAX;      //f: 0-2p
                double th = acos(2.0 * rand() / RAND_MAX-1.0);      //th:0-p

                press_map << x << "\t" << y << "\t" << z << "\t" << f << "\t" << th << endl;          
            }
        }
    }
}

using namespace chic;
int main(int argc, char* argv[])
{
    if (argc < 3) {
        printf("Usage: bm <in-channel> <out-channel>");
        return 1;
    }

    Comm::init();

    Comm com;

    com.register_input_channel("in", argv[1]);
    com.register_output_channel("out", argv[2]);

    while (true) {
        InChannel in = com.get_input_channel("in");
        Message m = in.get();
        string msg(m.payload().data(), m.payload().size());

        printf("Got message with id=%d and body=%s\n", m.id(), msg.c_str());

        auto ix = msg.find(':');
        int radius = atoi( msg.substr(0, ix).c_str());
        string conc_map = msg.substr(ix+1);


        char filename[L_tmpnam];
        tmpnam (filename);
        printf("Saving presure map in file %s\n", filename);
        compute_press_map(filename, radius);
        OutChannel out = com.get_output_channel("out");
        out.put(filename);
    }

    return 0;
}
