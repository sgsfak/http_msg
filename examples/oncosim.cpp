#include "oncosim_state.h"
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include "chic_comm.h"

using namespace std;


State tumor_initialization(int tumor_radius)
{
    State state(tumor_radius);

    for(int i=0; i<5;i++){
        state.stem_pop[i].set_all(100);
        state.limp_pop[i].set_all(100);
        state.stem_time[i].set_all(10);
        state.limp_time[i].set_all(10);
    }
    state.diff_pop.set_all(50);
    state.a_pop.set_all(5);
    state.n_pop.set_all(150);

    return state;
}


void save_tumor_concentration(const string& filename, const State& state)
{

    ofstream tum_conc_map(filename);  

    int tumor_radius = state.tumor_radius;

    for (int x=0;x< tumor_radius;x++){      
        for (int y=0;y< tumor_radius;y++){      
            for (int z=0;z< tumor_radius;z++){
                double cell_pop = state.a_pop.at(x,y,z) + state.n_pop.at(x,y,z) + state.diff_pop.at(x,y,z);

                for (int m=0;m<5;m++)
                    cell_pop += state.stem_pop[m].at(x,y,z) + state.limp_pop[m].at(x,y,z);
                tum_conc_map << x << "\t" << y << "\t" << z << "\t" << cell_pop << endl;          
            }
        }
    }


}

void first_mesh_scan(State& s)
{
// no-op
}

void second_mesh_scan(State& s)
{
// no-op
}

void update_state(State& s, const char*)
{
// no-op
}

void call_BM(const string& conc_map_filename)
{

    // HOW???
}

string read_BM()
{
    // HOW ???
    return "";
}

State update_state(State& state, const string& pressure_map_filename)
{
    // no-op
    return state;
}

int main(int argc, char* argv[])
{
    if (argc < 5) {
        cout << "Usage: os <tumor_radius> <tmax> <in-channel> <out-channel>\n";
        return 1;
    }


    chic::Comm::init();
    chic::Comm com;
    com.register_output_channel("to-bm", argv[4]);
    com.register_input_channel("from-bm", argv[3]);



    int radius = atoi(argv[1]);
    int tmax = atoi(argv[2]);
    cout << "Starting simulation with radius=" << radius << " for " << tmax << " hours" << endl;

    State S = tumor_initialization(radius);
    int t = 0; // in hours
    auto to_bm = com.get_output_channel("to-bm");
    auto from_bm = com.get_input_channel("from-bm");
    while (true) {

        first_mesh_scan(S);
        second_mesh_scan(S);

        const int day = 24 ; // one day, in hours
        if (t % day == 0) {
            // one day passed, so call BM
            string conc_map = "concentration_map.dat";
            save_tumor_concentration(conc_map, S);

            cout << "Now at day " << (t / day + 1) << "... I need to call Biomechanical model.." << endl;
            /*
            call_BM( conc_map ); // send tumor concentration
            string lpm = read_BM(); // read direction of least pressure map
            */

            stringstream ss;
            ss << S.tumor_radius << ":" << conc_map;
            string msg = ss.str();
            cout << "Sending " << msg << endl;
            to_bm.put(ss.str());
            cout << "Now waiting " << endl;
            auto m = from_bm.get();
            string lpm = m.payload().data();
            cout << "Got " << lpm << " from BM!" << endl;

            S = update_state(S, lpm);
        }
        if (t >= tmax)
            break;
        t += 1;
    }


    cout << "Ended..." << endl;

    return 0;
}
