#include "tightsketch_simd.hpp"
#include "nmmintrin.h"
#include "immintrin.h"

tightSketchSIMD::tightSketchSIMD(int depth, int width, int lgn) {

    ss_.depth = depth;
    ss_.width = width;
    ss_.lgn = lgn;
    ss_.sum = 0;
    ss_.counts = new SBucket *[depth*width];
    for (int i = 0; i < depth*width; i++) {
        ss_.counts[i] = (SBucket*)calloc(1, sizeof(SBucket));
        memset(ss_.counts[i], 0, sizeof(SBucket));
    }
    ss_.hash = new unsigned long[depth];
    ss_.scale = new unsigned long[depth];
    ss_.hardner = new unsigned long[depth];
    char name[] = "tightSketchSIMD";
    unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
    for (int i = 0; i < depth; i++) {
        ss_.hash[i] = GenHashSeed(seed++);
    }
    for (int i = 0; i < depth; i++) {
        ss_.scale[i] = GenHashSeed(seed++);
    }
    for (int i = 0; i < depth; i++) {
        ss_.hardner[i] = GenHashSeed(seed++);
    }
}

tightSketchSIMD::~tightSketchSIMD() {
    for (unsigned i = 0; i < ss_.depth*ss_.width; i++) {
        free(ss_.counts[i]);
    }
    delete [] ss_.hash;
    delete [] ss_.scale;
    delete [] ss_.hardner;
    delete [] ss_.counts;
}




void tightSketchSIMD::Update(unsigned char* key, val_tp val) {
    int keylen = ss_.lgn/8; long min = 99999999; int loc=-1; int indicator=0;
    uint32_t onehash[4];
    MurmurHash3_x64_128(key, keylen, ss_.hardner[0], onehash); 
    __m256i vhash = _mm256_set_epi32(0, onehash[0], 0, onehash[1], 0, onehash[2], 0, onehash[3]); 
    __m256i vwidth = _mm256_set1_epi32(ss_.width);
    __m256i vbucket = _mm256_srli_epi64(_mm256_mul_epu32(vhash, vwidth), 32);
    __m256i vrow = _mm256_set_epi32(0, 0, 0, 1, 0, 2, 0, 3);
    union {__m256i vindex; unsigned long index[4];};
    vindex = _mm256_add_epi64(vbucket, _mm256_mul_epu32(vrow, vwidth));
    tightSketchSIMD::SBucket* sbucket[4] = {ss_.counts[index[0]], ss_.counts[index[1]], ss_.counts[index[2]], ss_.counts[index[3]]}; 
    __m256i vkey = _mm256_set_epi64x(*((uint64_t *)(sbucket[3]->key)), *((uint64_t *)(sbucket[2]->key)), *((uint64_t *)(sbucket[1]->key)), *((uint64_t *)(sbucket[0]->key))); 
    __m256i comkey = _mm256_set1_epi64x(*((uint64_t *)key)); 
    union {__m256i vflag; uint64_t flag[4];};
    vflag = _mm256_cmpeq_epi64 (vkey, comkey);
    for (int i = 0; i < 4; i++) {
        if (flag[i] != 0) { 
             	sbucket[i]->count += 1;
                sbucket[i]->hotcount += 1; 
                indicator = 1;
                return;
        } else { 
           if(sbucket[i]->key == NULL && sbucket[i]->count==0) 
           {
           	   memcpy_8(sbucket[i]->key, key);
           			sbucket[i]->count = 1;
                    sbucket[i]->hotcount = 1;
           			indicator=1;
                    return;
           }
        }
    }

          if (indicator==0) 
           {
              for (int j = 0; j < 4; j++) {
               if (sbucket[j]->count < min)
               {
                  min = sbucket[j]->count;
                  loc = j;
               }
           }
           if(loc>=0){
                      sbucket[loc]->hotcount-=1;
                      if(sbucket[loc]->count<10)
                      {
                      int k = rand() % (int)((sbucket[loc]->count) + 1.0) + 1.0;
                      if (k > (sbucket[loc]->count)) 
                      		{
                      			sbucket[loc]->count -= 1;
                                if(sbucket[loc]->count<=0)
                                {
                                   memcpy_8(sbucket[loc]->key, key);
                                   sbucket[loc]->count=1;                      
                                }
                      		}
                      }
                      else
                      {
                        int j = rand() % (int)((sbucket[loc]->count * sbucket[loc]->hotcount)+1.0)+1.0;
                        if(j>(int)sbucket[loc]->count * sbucket[loc]->hotcount)
                      		{
                      			sbucket[loc]->count -= 1;
                                if(sbucket[loc]->count<=0)
                                {
                                   memcpy_8(sbucket[loc]->key, key);
                                   sbucket[loc]->count=1;                      
                                }
                      		}
                      }

           }
        }
}


void tightSketchSIMD::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> >&results) {

    myset res;
    for (unsigned i = 0; i < ss_.width*ss_.depth; i++) {
        if (ss_.counts[i]->count > (int)thresh) {
            key_tp reskey;
            memcpy(reskey.key, ss_.counts[i]->key, ss_.lgn/8);
            res.insert(reskey);
        }
    }

    for (auto it = res.begin(); it != res.end(); it++) {
        val_tp resval = 999999999;
        val_tp max = 0;
        uint32_t onehash[4];
        MurmurHash3_x64_128( (*it).key, ss_.lgn/8, ss_.hardner[0], onehash);
        __m256i vhash = _mm256_set_epi32(0, onehash[0], 0, onehash[1], 0, onehash[2], 0, onehash[3]);
        __m256i vwidth = _mm256_set1_epi32(ss_.width);
        __m256i vbucket = _mm256_srli_epi64(_mm256_mul_epu32(vhash, vwidth), 32);
        __m256i vrow = _mm256_set_epi32(0, 0, 0, 1, 0, 2, 0, 3);
        union {__m256i vindex; unsigned long index[4];};
        vindex = _mm256_add_epi64(vbucket, _mm256_mul_epu32(vrow, vwidth));

        for (unsigned j = 0; j < ss_.depth; j++) {
            if (memcmp(ss_.counts[index[j]]->key, (*it).key, ss_.lgn/8) == 0) {
                max += ss_.counts[index[j]]->count; break;
            }
        }
        if (max != 0) resval = max;

            key_tp key;
            memcpy(key.key, (*it).key, ss_.lgn/8);
            std::pair<key_tp, val_tp> node;
            node.first = key;
            node.second = resval;
            results.push_back(node);

    }
}



val_tp tightSketchSIMD::PointQuery(unsigned char* key) {
    return Up_estimate(key);
}

val_tp tightSketchSIMD::Low_estimate(unsigned char* key) {
        val_tp max = 0;
	for (unsigned i = 0; i < ss_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, ss_.lgn / 8, ss_.hardner[i]) % ss_.width;

		unsigned long index = i * ss_.width + bucket;
		if (memcmp(ss_.counts[index]->key, key, ss_.lgn / 8) == 0)
		{
			max += ss_.counts[index]->count;
		}
		index = i * ss_.width + (bucket + 1) % ss_.width;
		if (memcmp(key, ss_.counts[i]->key, ss_.lgn / 8) == 0)
		{
			max += ss_.counts[index]->count;
		}

	}
	return max;
}

val_tp tightSketchSIMD::Up_estimate(unsigned char* key) {
        int max = 0, min = 999999999;
	for (unsigned i = 0; i < ss_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, ss_.lgn / 8, ss_.hardner[i]) % ss_.width;

		unsigned long index = i * ss_.width + bucket;
		if (memcmp(ss_.counts[index]->key, key, ss_.lgn / 8) == 0)
		{
			max += ss_.counts[index]->count;
		}
		if (ss_.counts[index]->count < min)min = ss_.counts[index]->count;
		index = i * ss_.width + (bucket + 1) % ss_.width;
		if (memcmp(key, ss_.counts[i]->key, ss_.lgn / 8) == 0)
		{
			max += ss_.counts[index]->count;
		}

	}
	if (max)return max;
	return min;

}

val_tp tightSketchSIMD::GetCount() {
    return ss_.sum;
}



void tightSketchSIMD::Reset() {
    ss_.sum=0;
    for (unsigned i = 0; i < ss_.depth*ss_.width; i++){
        ss_.counts[i]->count = 0;
        memset(ss_.counts[i]->key, 0, ss_.lgn/8);
    }
}

