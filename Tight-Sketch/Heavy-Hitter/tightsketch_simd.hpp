#ifndef SSKETCH_SIMD_H
#define SSKETCH_SIMD_H
#include <vector>
#include <unordered_set>
#include <utility>
#include <cstring>
#include <cmath>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "datatypes.hpp"
extern "C"
{
#include "hash.h"
#include "util.h"
}

class tightSketchSIMD {

    typedef struct SBUCKET_type {

        int count;
        int hotcount;
        unsigned char key[LGN];

    } SBucket;

    struct SS_type {

        //Counter to count total degree
        val_tp sum;
        //Counter table
        SBucket **counts;

        //Outer sketch depth and width
        unsigned depth;
        unsigned width;

        //# key word bits
        int lgn;

        unsigned long *hash, *scale, *hardner;
    };


    public:
    tightSketchSIMD(int depth, int width, int lgn);

    ~tightSketchSIMD();

    void Update(unsigned char* key, val_tp value);

    val_tp PointQuery(unsigned char* key);

    void Query(val_tp thresh, myvector& results);

    val_tp Low_estimate(unsigned char* key);

    val_tp Up_estimate(unsigned char* key);

    val_tp GetCount();

    void Reset();

    private:

    SS_type ss_;
};

#endif
