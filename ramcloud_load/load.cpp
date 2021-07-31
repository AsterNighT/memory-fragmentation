#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <random>
#include <cassert>

using uint = unsigned int;
// Bound for phase one, the program would allocate this much memory first.
constexpr const size_t size10G = 10*1024*1024*1024UL;
constexpr const size_t size10GInBits = size10G * 8;

// After that, the program would keep allocating and deleting existing object at random to keep the total memory below size10G. It stops when a total of size50G memory has been allocated.
constexpr const size_t size50G = 50*1024*1024*1024UL;
constexpr const size_t size50GInBits = size50G * 8;

// Minimal size allocated, used to calculate bookkeeping size.
constexpr const size_t minimalSize = 50UL;

// Size range for allocation above. If Lower==Upper, the size is fixed, otherwise uniformly distributed.
constexpr const size_t formerSizeRangeLower[8] = {100,100,100,100,100,100,1000,50};
constexpr const size_t formerSizeRangeUpper[8] = {100,100,100,150,150,200,2000,150};

// If a experiment has phase two, it will delete a fraction of existing object, and switch to another size range.
constexpr const bool hasPhaseTwo[8] = {false,true,true,true,true,true,true,true};
constexpr const double deleteFraction[8] = {0,0,0.9,0,0.9,0.5,0.9,0.9};

// Size range for allocation at the second phase
constexpr const size_t latterSizeRangeLower[8] = {0,130,130,200,200,1000,1500,5000};
constexpr const size_t latterSizeRangeUpper[8] = {0,130,130,250,250,2000,2500,15000};

// A struct for allocation. An extra field is needed to track memory usage.
struct Record{
    Record():data(nullptr),size(0){}
    void* data;
    size_t size;
};

// We would need tons of memory to keep track of allocation. Would be better if we keep it global so that it would be in data segment, rather than interfering the allocator. This yields a memory usage about 3.2G. We would substract this part in the final report.
Record mem[size10G/minimalSize+2] = {};

// Read /proc/self/status for memory usage. Since we cannot parse it easily we would simply print it. For the same reason this cannot work on windows
void GetMeminfo(){
    std::ifstream f("/proc/self/status");

    if (f.is_open())
        std::cout << f.rdbuf();
}

void StartExperiment(int no){
    size_t totalUsage = 0UL;
    size_t footprint = 0UL;
    uint cnt;

    // Phase one
#if defined(DEBUG)
    printf("Stepping into phase one for experiment no %d\n", no);
#endif
    std::default_random_engine generator;
    std::uniform_int_distribution<size_t> formerDistribution(formerSizeRangeLower[no],formerSizeRangeUpper[no]);

    while(totalUsage<size10G){
        uint size = formerDistribution(generator);
        mem[cnt].data = malloc(size*8);
        mem[cnt].size = size;
        cnt++;
        totalUsage+=size;
        footprint+=size;
    }

    // Try to delete and reallocate object while keep the usage below 10G.
    std::uniform_int_distribution<uint> randomSelection(0,cnt-1);
    while(footprint<size50G){
        uint size = formerDistribution(generator);
        uint pos = randomSelection(generator);
        if(mem[pos].data==nullptr) {
            free(mem[pos].data);
            totalUsage -= mem[pos].size;
            mem[pos].data = nullptr;
            mem[pos].size = 0;
        }
        if (totalUsage + size < size10G){
            mem[pos].data = malloc(size*8);
            mem[pos].size = size;
            totalUsage += size;
            footprint+=size;
        }
    }

    // It should work, but let's be sure.
    assert(totalUsage < size10G);
#if defined(DEBUG)
    GetMeminfo();
#endif
    // Phase two
    if(hasPhaseTwo[no]){
#if defined(DEBUG)
        printf("Stepping into phase two for experiment no %d\n", no);
#endif
        // Clear the footprint
        footprint = 0;
        uint objectsToDelete = floor(deleteFraction[no]*cnt);
        uint objectsDeleted = 0;
        std::uniform_int_distribution<size_t> latterDistribution(latterSizeRangeLower[no],latterSizeRangeUpper[no]);
        while(objectsDeleted<objectsToDelete){
            uint pos = randomSelection(generator);
            if(mem[pos].data == nullptr) continue;
            free(mem[pos].data);
            totalUsage -= mem[pos].size;
            mem[pos].data = nullptr;
            mem[pos].size = 0;
            objectsDeleted++;
        }
        
        // Try to delete and reallocate object while keep the usage below 10G. Same as above except for object size. This might be very slow since a lot of objects have to be deleted before you can allocate a big object. And the array will get sparser and sparser, making it more difficult to find some thing to delete.
        while(footprint<size50G){
            uint size = latterDistribution(generator);
            uint pos = randomSelection(generator);
            if(mem[pos].data == nullptr) continue;
            if(mem[pos].data==nullptr) {
                free(mem[pos].data);
                totalUsage -= mem[pos].size;
                mem[pos].data = nullptr;
                mem[pos].size = 0;
            }
            if (totalUsage + size < size10G){
                mem[pos].data = malloc(size*8);
                mem[pos].size = size;
                totalUsage += size;
                footprint+=size;
            }
        }
    }

}

int main() {
    int no;
    std::cin >> no;
    switch(no){
        case 0 ... 7: // experiment
            StartExperiment(no);
            break;
        default: // collect basic memory usage
            break;
    }
    GetMeminfo();
}