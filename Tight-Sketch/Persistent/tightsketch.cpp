#include "tightsketch.hpp"
#include <math.h>

tightSketch::tightSketch(int depth, int width, int lgn) {

	tight_.depth = depth;
	tight_.width = width;
	tight_.lgn = lgn;
	tight_.sum = 0;
	tight_.counts = new SBucket *[depth*width];
	for (int i = 0; i < depth*width; i++) {
		tight_.counts[i] = (SBucket*)calloc(1, sizeof(SBucket));
		memset(tight_.counts[i], 0, sizeof(SBucket));
		tight_.counts[i]->key[0] = '\0';
	}

	tight_.hash = new unsigned long[depth];
	tight_.scale = new unsigned long[depth];
	tight_.hardner = new unsigned long[depth];
	char name[] = "tightSketch";
	unsigned long seed = AwareHash((unsigned char*)name, strlen(name), 13091204281, 228204732751, 6620830889);
	for (int i = 0; i < depth; i++) {
		tight_.hash[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		tight_.scale[i] = GenHashSeed(seed++);
	}
	for (int i = 0; i < depth; i++) {
		tight_.hardner[i] = GenHashSeed(seed++);
	}
}

tightSketch::~tightSketch() {
	for (int i = 0; i < tight_.depth*tight_.width; i++) {
		free(tight_.counts[i]);
	}
	delete[] tight_.hash;
	delete[] tight_.scale;
	delete[] tight_.hardner;
	delete[] tight_.counts;
}

void tightSketch::Update(unsigned char* key, val_tp val) {
	unsigned long bucket = 0;
	int keylen = tight_.lgn / 8;tight_.sum += 1;
	tightSketch::SBucket *sbucket;
	int flag = 0;
	long min = 99999999; int loc = -1; int k; int index; 
	for (int i = 0; i < tight_.depth; i++) { 
		bucket = MurmurHash64A(key, keylen, tight_.hardner[i]) % tight_.width;
		index = i * tight_.width + bucket;
		sbucket = tight_.counts[index]; 
		if (sbucket->key[0] == '\0'&&sbucket->count==0&&sbucket->status==0) { 
			memcpy(sbucket->key, key, keylen);
			flag = 1; 
			sbucket->status = 1; 
			sbucket->count = 1;
            return;
		}
		else if (memcmp(key, sbucket->key, keylen) == 0) { 
            if (sbucket->status==0)
			{
              flag = 1; 
			  sbucket->count += 1;
              sbucket->status=1; 
            }
            return;
		}
        if (sbucket->count < min) 
		{
			min = sbucket->count;
			loc = index; 
		}
	}
	if (flag == 0 && loc >= 0) 
	{
		sbucket = tight_.counts[loc]; 
		
                if(sbucket->status==1) {return;}
                
                if(sbucket->count<10) 
                {
                  k = rand() % (int)((sbucket->count)+1.0) + 1.0;
                  if(k > (int)((sbucket->count)))
                      sbucket->count-=1; 
                  if(sbucket->count<=0)
                  {
                    memcpy(sbucket->key,key,keylen);
			        sbucket->count=1;
			        sbucket->status=1;                    
                  }
                }               
                else 
                {
                 int j = rand() % (int)((sbucket->hotcount*sbucket->count)+1.0) + 1.0;
                 if (j>(int)(sbucket->hotcount*sbucket->count))
                 {
                  sbucket->count-=1;  
                  if(sbucket->count<=0)
                  {
                      memcpy(sbucket->key,key,keylen);
			          sbucket->count = 1;
			          sbucket->status=1;                    
                  }
                 }
                }                
	}
}

void tightSketch::Query(val_tp thresh, std::vector<std::pair<key_tp, val_tp> >&results) {
	myset res; 
	for (int i = 0; i < tight_.width*tight_.depth; i++) {
                     
		if (tight_.counts[i]->count > (int)thresh) {
			key_tp reskey; 
			memcpy(reskey.key, tight_.counts[i]->key, tight_.lgn / 8);
			res.insert(reskey);

		}

	}
	for (auto it = res.begin(); it != res.end(); it++) {
		val_tp resval = 99999999; val_tp max = 0;
		for (int j = 0; j < tight_.depth; j++) {
			unsigned long bucket = MurmurHash64A((*it).key, tight_.lgn / 8, tight_.hardner[j]) % tight_.width;
			unsigned long index = j * tight_.width + bucket;
			if (memcmp(tight_.counts[index]->key, (*it).key, tight_.lgn / 8) == 0) {
				max += tight_.counts[index]->count;			
			}
			
		}
		if (max != 0)resval = max;

			key_tp key;
			memcpy(key.key, (*it).key, tight_.lgn / 8);
			std::pair<key_tp, val_tp> node;
			node.first = key;
			node.second = resval;
			results.push_back(node);
	}
	std::cout << "results.size = " << results.size() << std::endl;
}


void tightSketch::QueryHotness(){ 
for (int i = 0; i < tight_.depth*tight_.width; i++){
        if(tight_.counts[i]->status == 0)
        {
            tight_.counts[i]->hotcount = tight_.counts[i]->hotcount - 1;
            if(tight_.counts[i]->hotcount<0)
            {tight_.counts[i]->hotcount = 0;}
        }
        if(tight_.counts[i]->status == 1)
        {
            tight_.counts[i]->hotcount = tight_.counts[i]->hotcount + 1;
        }
	}
}

void tightSketch::NewWindow(){ 
for (int i = 0; i < tight_.depth*tight_.width; i++){
			tight_.counts[i]->status = 0;
	}
}

val_tp tightSketch::PointQuery(unsigned char* key) {
	return Low_estimate(key);
}

val_tp tightSketch::Low_estimate(unsigned char* key) {
    val_tp max = 0;
	for (int i = 0; i < tight_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, tight_.lgn / 8, tight_.hardner[i]) % tight_.width;

		unsigned long index = i * tight_.width + bucket;
		if (memcmp(tight_.counts[index]->key, key, tight_.lgn / 8) == 0)
		{
			max += tight_.counts[index]->count;
		}
		index = i * tight_.width + (bucket + 1) % tight_.width;
		if (memcmp(key, tight_.counts[i]->key, tight_.lgn / 8) == 0)
		{
			max += tight_.counts[index]->count;
		}

	}
	return max;
}

val_tp tightSketch::Up_estimate(unsigned char* key) {
    unsigned max = 0, min = 999999999;
	for (int i = 0; i < tight_.depth; i++) {
		unsigned long bucket = MurmurHash64A(key, tight_.lgn / 8, tight_.hardner[i]) % tight_.width;

		unsigned long index = i * tight_.width + bucket;
		if (memcmp(tight_.counts[index]->key, key, tight_.lgn / 8) == 0)
		{
			max += tight_.counts[index]->count;
		}
		if (tight_.counts[index]->count < min) min = tight_.counts[index]->count;
		index = i * tight_.width + (bucket + 1) % tight_.width;
		if (memcmp(key, tight_.counts[i]->key, tight_.lgn / 8) == 0)
		{
			max += tight_.counts[index]->count;
		}

	}
	if (max)return max;
	return min;

}

val_tp tightSketch::GetCount() {
	return tight_.sum;
}



void tightSketch::Reset() {
	tight_.sum = 0;
	for (int i = 0; i < tight_.depth*tight_.width; i++) {
		tight_.counts[i]->count = 0;
		memset(tight_.counts[i]->key, 0, tight_.lgn / 8);
	}
}

void tightSketch::SetBucket(int row, int column, val_tp sum, long count, unsigned char* key) {
	int index = row * tight_.width + column;
	tight_.counts[index]->count = count;
	memcpy(tight_.counts[index]->key, key, tight_.lgn / 8);
}

tightSketch::SBucket** tightSketch::GetTable() {
	return tight_.counts;
}





