#include "heavy_changer.hpp"
#include "adaptor.hpp"
#include <unordered_map>
#include <utility>
#include "util.h"
#include "datatypes.hpp"
#include <iomanip>

typedef std::unordered_map<key_tp, val_tp*, key_tp_hash, key_tp_eq> groundmap;


int main(int argc, char* argv[]) {

    int memory_size;
    std::cin>>memory_size;
    int width = memory_size*1024/(16*4);

    //Dataset list
    const char* filenames = "iptraces.txt";
    unsigned long long buf_size = 5000000000;

    double thresh = 0.0008;

    //tight sketch parameters
    int tight_width = width; //number of buckets in each row
    int tight_depth = 4; //number of rows 1


    int numfile = 0;
    double precision=0, recall=0, error=0, throughput=0, detectime=0;
    double avpre=0, avrec=0, averr=0, avthr=0, avdet=0;

    std::string file;

    std::ifstream tracefiles(filenames);
    if (!tracefiles.is_open()) {
        std::cout << "Error opening file" << std::endl;
        return -1;
    }
    groundmap groundtmp;
    mymap ground;
    tuple_t t;
    memset(&t, 0, sizeof(tuple_t));
    val_tp diffsum = 0;

    //tightSketch
    HeavyChanger<tightSketch>* heavychangertight = new HeavyChanger<tightSketch>(tight_depth, tight_width, LGN*8);
    for (std::string file; getline(tracefiles, file);) {
        //load traces and get ground

        Adaptor* adaptor =  new Adaptor(file, buf_size);
        std::cout << "[Dataset]: " << file << std::endl;
        memset(&t, 0, sizeof(tuple_t));

        //Get ground
        adaptor->Reset();

        //Reset gounrdtmp;
        for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
            it->second[1] = it->second[0];
            it->second[0] = 0;
        }

        //insert hash table
        while(adaptor->GetNext(&t) == 1) {
            key_tp key;
            memcpy(key.key, &(t.key), LGN);
            if (groundtmp.find(key) != groundtmp.end()) {
                groundtmp[key][0] += 1;
            } else {
                val_tp* valtuple = new val_tp[2]();
                groundtmp[key] = valtuple;
                groundtmp[key][0] += 1;
            }
        }

        if (numfile != 0) {
            ground.clear();
            val_tp oldval, newval, diff;
            diffsum = 0;

            for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
                oldval = it->second[0];
                newval = it->second[1];
                diff = newval > oldval ? newval - oldval : oldval - newval;
                diffsum += diff;
            }

            for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
                oldval = it->second[0];
                newval = it->second[1];
                diff = newval > oldval ? newval - oldval : oldval - newval;
                if (diff > thresh*(diffsum)){
                    ground[it->first] = diff;
                }
            }
        }

        
        //Update sketch
        uint64_t t1, t2;

        adaptor->Reset();
        heavychangertight->Reset();
        tightSketch* cursk = (tightSketch*)heavychangertight->GetCurSketch();
        t1 = now_us();
        while(adaptor->GetNext(&t) == 1) {
            cursk->Update((unsigned char*)&(t.key), 1);
        }
        t2 = now_us();
        throughput = adaptor->GetDataSize()/(double)(t2-t1)*1000000000;

        if (numfile != 0) {
            std::vector<std::pair<key_tp, val_tp> > results;
            //Query
            results.clear();
            val_tp threshold = thresh*diffsum;
            t1 = now_us();
            heavychangertight->Query(threshold, results);
            t2 = now_us();
            detectime = (double)(t2-t1)/1000000000;
            //Evaluate accuracy

            int tp = 0, cnt = 0;;
            error = 0;
            for (auto it = ground.begin(); it != ground.end(); it++) {
                if (it->second > threshold) {
                    cnt++;
                    for (auto res = results.begin(); res != results.end(); res++) {
                        if (memcmp(it->first.key, res->first.key, sizeof(res->first.key)) == 0) {
                            double hh=res->second > it->second?res->second - it->second: it->second-res->second;
error += hh*1.0/it->second;
                            tp++;
                        }
                    }
                }
            }
            precision = tp*1.0/results.size();
            recall = tp*1.0/cnt;
            error = error/tp;      
        }
        avpre += precision; avrec += recall; averr += error; avdet += detectime; avthr += throughput;
        numfile++;
        delete adaptor;
    }

    //Delete
    for (auto it = groundtmp.begin(); it != groundtmp.end(); it++) {
        delete [] it->second;
    }
    delete heavychangertight;


    std::cout << "-----------------------------------------------   Summary    -------------------------------------------------------" << std::endl;
        //Output to standard output
        std::cout << std::setfill(' ');
        std::cout << std::setw(20) << std::left << "Sketch"
            << std::setw(20) << std::left << "Precision"
            << std::setw(20) << std::left << "Recall" << std::setw(20)
            << std::left << "Relative Error" << std::setw(20) << std::left << "Throughput" << std::setw(20)
            << std::left << "Detection Time" << std::endl;
            std::cout << std::setw(20) <<  "tightSketch";
            std::cout  << std::setw(20) << std::left << avpre/(numfile-1) << std::setw(20) << std::left << avrec/(numfile-1) << std::setw(20)
                << std::left << averr/(numfile-1) << std::setw(20) << std::left << avthr/(numfile) << std::setw(20)
                << std::left << avdet/(numfile-1) << std::endl;
}
