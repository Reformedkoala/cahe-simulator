#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <bit>
#include <ctime>

using namespace std;


class cacheSet{
	public:
		class way{
		public:
			//Valid bit
			bool valid;
			//tag bits
			int tag;
			//data bits
			uint32_t data;
			//priority for LRU or NMRU
			int priority;
			way(){
				//always initialize to 0
				valid = 0;
				//always initialize to null
				tag = 0;
				//always initialize to null
				data = 0;	
				//Each starts at 0
				priority = 0;
			}
		};
		//Vector to store each way of a set
		vector<way> ways;
		cacheSet(){}
		cacheSet(int numWays){
			ways.resize(numWays);
		}
	};


class cache{
public:
	cache(string traceFile, unsigned int constructNumWays, unsigned int constructCacheSize, unsigned int constructBlockSize, string constructReplacementPolicy){
		infile.open(traceFile);
		// Initializing Values from function
		numWays = constructNumWays;
		replacementPolicy = constructReplacementPolicy;
		cacheSize = constructCacheSize*1000;
		blockSize = constructBlockSize*4;
		//Doing TIO math via popcount feature
		unsigned int blockBits = __popcount(blockSize - 1);
		numberSets = cacheSize/(blockSize*numWays);
		setOffset = blockBits;
		setMask = numberSets-1;
		unsigned int setBits = __popcount(setMask);
		tagOffset = blockBits + setBits;

		//Initializing our sets to have correct number of ways and sets
		for( int i = 0; i < numberSets; i++){
			cacheSet tempSet(numWays);
			sets.push_back(tempSet);
		}


		//Determining replacement policy
		if( replacementPolicy == "LRU" ){
			LRU = 1;
		}else if (replacementPolicy == "Random"){
			Random = 1;
		} else{
			NMRURandom = 1;
		}
		prevAddress=1;
	}

	~cache(){
		infile.close();

	}

	int getSet(uint32_t address){
		int shiftedAddress = address >> setOffset;
  		return shiftedAddress & setMask;
	}

	int getTag(uint32_t address){
		return address >> tagOffset;
	}

	bool simAccess(uint32_t address){
		bool hit = false;
		int invalid = -1;
		int set = getSet(address);
		int tag = getTag(address);
	    //cout << address << " " << set << " " << tag <<endl;
		for (unsigned int i = 0; i < sets[set].ways.size(); i++){
	        // Check if the way is invalid
	        if (!sets[set].ways[i].valid){
	          	// Keep track of invalid entries in case we need them
	          	invalid = i;
	          	continue;
	        }

	        // Check if the tag matches
	        if (tag != sets[set].ways[i].tag) {
	      	  	continue;
	        }

	        // We found the line, so mark it as a hit
	        hit = true;
	        // Break out of the loop
	        break;
	    }

	    //Invalid line base case
	    if (!hit && invalid >= 0) {
		    sets[set].ways[invalid].valid = 1;
		    sets[set].ways[invalid].data = address;
		    sets[set].ways[invalid].tag = tag;
		    return hit;
		}

		//Replacement policy boolean 
	    if(!hit && LRU){
	    	this->LRUReplace(address, set, tag);
	    	return hit;

	    } else if(!hit && Random){
	    	this->RANDOMReplace(address, set, tag);
	    	return hit;

	    }else if (!hit && NMRURandom){
	    	this->NMRURandomReplace(address, set, tag, prevAddress);
	    	prevAddress = address;
	    	return hit;
	    	
	    }else{
	    	prevAddress = address;
			return hit;

	    }
	}


	void LRUReplace(uint32_t address, int set, int tag){
		int maximum = 0;
		int maximumIndice = 0;
		//cout << address << " " << set << " " << tag <<endl;
		for(int i = 0; i < sets[set].ways.size(); i++){
			if(sets[set].ways[i].priority != 0 && sets[set].ways[i].valid == 1){
				if(maximum < sets[set].ways[i].priority){
					maximum = sets[set].ways[i].priority;
					maximumIndice = i;
				}
				sets[set].ways[i].priority++;
			}
		}
		sets[set].ways[maximumIndice].tag = tag;
		sets[set].ways[maximumIndice].valid = 1;
		sets[set].ways[maximumIndice].priority = 1;
		sets[set].ways[maximumIndice].data = address;
		//cout << address << endl;

	}


	void RANDOMReplace(uint32_t address, int set, int tag){
    	int currRand = rand()%sets[set].ways.size();
    	sets[set].ways[currRand].tag = tag;
    	sets[set].ways[currRand].valid = 1;
		sets[set].ways[currRand].priority = 1;
		sets[set].ways[currRand].data = address;

	}


	void NMRURandomReplace(uint32_t address, int set, int tag, uint32_t prevAddress){
		int currRand = rand()%sets[set].ways.size();
		
		while(sets[set].ways[currRand].data == prevAddress){
			currRand = rand()%sets[set].ways.size();
		}
		
    	sets[set].ways[currRand].tag = tag;
    	sets[set].ways[currRand].valid = 1;
		sets[set].ways[currRand].priority = 1;
		sets[set].ways[currRand].data = address;
	}

	void run() {
    	// Keep reading data from a file
    	string line;
    	bool hit;
    	while (getline(infile, line)) {
      		// Get the data for the access
      		int address = stoi(line);

      		// Perform the cache access
      		hit = simAccess(address);
      		totalAccesses++;
      		if(hit == true){
      			cacheHits++;
      		}else{
      			cacheMisses++;
      		}
    	}
    	this->printStats();
  	}

  	void printStats(){
  		cout << "Cache_Size: " << cacheSize/1000 << " KBytes" << endl;
		cout << "Block_Size: " << blockSize/4 << " words (" << blockSize << " bytes)" << endl;
		cout << "Associativity: " << numWays << " way" << endl;
		cout << "Replacement_Policy: " << replacementPolicy << endl;
		cout << "Total_Number_of_Accesses: " << totalAccesses << endl;
		cout << "Cache_Hits: " << cacheHits << endl;
		cout << "Cache_Misses: " << cacheMisses << endl;
		cout << "Cache_Hit_Rate: " << double(cacheHits)/double(totalAccesses) << endl;
		cout << "Cache_Miss_Rate: " << double(cacheMisses)/double(totalAccesses) << endl;
  	}

	void printCacheTIO(){
		cout << cacheSize << endl;
		cout << blockSize << endl;
		cout << numberSets << endl;
		cout << blockBits << endl;
		cout << setOffset << endl;
		cout << setBits << endl;
		cout << tagOffset << endl;
		cout << setMask << endl;
		cout << sets.size() << endl;
		cout << sets.at(0).ways.size();
	}

	vector<cacheSet> sets;
	unsigned int numWays;
	bool LRU;
	bool Random;
	bool NMRURandom;
	unsigned int cacheSize;
	unsigned int blockSize;
	unsigned int numberSets;
	unsigned int blockBits;
	unsigned int setOffset;
	unsigned int setBits;
	unsigned int tagOffset;
	unsigned int setMask;
	unsigned int cacheHits;
	unsigned int cacheMisses;
	unsigned int totalAccesses;
	string replacementPolicy;
	ifstream infile;
	uint32_t prevAddress;

};


int main(){
	srand(time(0));
	unsigned int associativeness = 16;
	string replacementPolicy = "LRU";
	unsigned int cacheSize = 128;
	unsigned int blockSize = 8;
	string traceFile = "tracefile.txt";
	/*
	cout << "Welcome to the cache simulator" << endl;
	cout << "Choose the associativeness of the cache(Power of 2 from 0 to 16): ";
	cin >> associativeness;
	cout << "Choose the Replcaement Policy of the cache(LRU, Random, or NMRURandom): ";
	getline(cin, replacementPolicy);
	cout << "Choose the size of the Cache(16, 32, 64, 128, or 256KB): ";
	cin >> cacheSize;
	cout << "Choose the block size of the cache(How many words): ";
	cin >> blockSize;
	//cout << "What is the name of the trace file to open: ";
	//getline(cin, traceFile);
	*/
	//cache* mainCache = new cache(traceFile, associativeness, cacheSize, blockSize, replacementPolicy);
	cache* mainCache = new cache(traceFile, 16, 128, 8, "LRU");
	//mainCache.printCacheTIO();
	mainCache->run();
	delete mainCache;


	return 0;
}