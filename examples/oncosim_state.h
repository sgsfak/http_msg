#ifndef ONCOSIM_STATE_H
#define ONCOSIM_STATE_H
#include <stdlib.h> 

template<class T>
struct Volume {
    private:
        int radius_;
        T * vals_;

    public:
        Volume(): radius_(0), vals_(0) {}
        void init(int size, T v = 0) {
            radius_ = size;
            size_t s = sizeof(T);
            int n = size * size * size;
            vals_ = static_cast<T*>( malloc(s * n));
            for (int i = 0; i<n; ++i)
                vals_[i] = v;
        }
        void free() {
            free(vals_);
            vals_ = 0;
            radius_ = 0;
        }
        T at(int x, int y, int z) const {
            int i =  x * radius_ * radius_ + y * radius_ + z;
            return vals_[i];
        }
        void set_at(int x, int y, int z, T v) {
            int i =  x * radius_ * radius_ + y * radius_ + z;
            vals_[i] = v;
        }
        void set_all(T v) {
            int n = radius_ * radius_ * radius_;
            for (int i=0; i<n; ++i)
                vals_[i] = v;
        }

};



struct State {

    int tumor_radius;

    Volume<double> stem_pop[5]; // [tumor_radius][ tumor_radius][tumor_radius][5];
    Volume<int> stem_time[5]; //[tumor_radius][ tumor_radius][tumor_radius] [5];
    Volume<double> limp_pop[5]; //[tumor_radius][ tumor_radius][tumor_radius] [5];
    Volume<int> limp_time[5]; //[tumor_radius][ tumor_radius][tumor_radius] [5];
    Volume<double> diff_pop; //[tumor_radius][ tumor_radius][tumor_radius];
    Volume<double> a_pop; //[tumor_radius][ tumor_radius][tumor_radius];
    Volume<double> n_pop; //[tumor_radius][ tumor_radius][tumor_radius];

    State(int radius): tumor_radius(radius)
    {
        for (int i =0; i<5; ++i) {
            stem_pop[i].init(tumor_radius);
            stem_time[i].init(tumor_radius);
            limp_pop[i].init(tumor_radius);
            limp_time[i].init(tumor_radius);
        }
        diff_pop.init(tumor_radius);
        a_pop.init(tumor_radius);
        n_pop.init(tumor_radius);
    }

};


#endif
