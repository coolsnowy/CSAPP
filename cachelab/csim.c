#include "cachelab.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <limits.h>

#define MAGIC_LRU_NUMBER 999

typedef struct {
    int valid;  // valid bit
    int tag;    // mark bit
    int lruNumber;  // the bit needed by lRU algorithm
} Line;

typedef struct {
    Line *lines;    // all lines in one set, which is E
} Set;

typedef struct {
    int setNumber;  // number of set
    int lineNumber; // number of line
    Set *sets;  // simulate cache
} SimCache;

/**
 * @brief printHelpMen
 */
void printHelpMen();

/**
 * @brief checkOptArg - check whethe the arguments is valid, and whether the arguments is start with '-'
 * @param curOptArg
 */
void checkOptArg(char *curOptArg);

/**
 * @brief getOpt - read the arguments from client
 * @param argc - number of arguments
 * @param argv - array of arguments
 * @param s - S = 2^s, number of set index bits
 * @param E - number of line in each Set
 * @param b - number of block bits
 * @param traceFileName - name of valgrind traces to replay
 * @param isVerbose - whether need to display trace information
 * @return
 */
int getOpt(int argc, char **argv, int *s, int *E, int *b, char *traceFileNmae, int *isVerbose);

/**
 * @brief initSimCache - initial the Simulate Cache
 * @param s
 * @param E
 * @param b
 * @param cache
 */
void initSimCache(int s, int E, int b, SimCache *cache);

/**
 * @brief getSet - get the set number from the address
 */
int getSet(int addr, int s, int b);

/**
 * @brief getTag -- find the tag number from the address
 */
int getTag(int addr, int s, int b);

/**
 * @brief loadData
 */
void loadData(SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose);

/**
 * @brief storeData
 */
void storeData(SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose);

/**
 * @brief modifyData
 */
void modifyData(SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose);

/**
 * @brief updateCache
 * return 1 means is full, need a sacrifice line
 */
int updateCache(SimCache *simCache, int setBits, int tagBits);

/**
 * @brief findMinLruNumber - return the minium Lru number of the set, and use it as sacrifice line
 */
int findMinLruNumber(SimCache *simCache, int setBits);

/**
 * @brief updateLruNumber - the hitIndex items update its lru number as MAGIC_LRU_NUMBER, all others'r lruNumber decrease 1
 */
void updateLruNumber(SimCache *simCache, int setBits, int hitIndex);


/**
 * @brief isMiss - judge whether hit
 */
int isMiss(SimCache *simCache, int setBits, int tagBits);

int misses;
int hits;
int evictions;

int main(int argc, char **argv) {
    int s = 0, E = 0, b = 0, isVerbose = 0;
    char traceFileName[100], opt[10];
    int addr = 0, size = 0;
    misses = hits = evictions = 0;
    SimCache cache;

    getOpt (argc, argv, &s, &E, &b, traceFileName, &isVerbose);
    initSimCache (s, E, b, &cache);
    FILE *traceFile = fopen(traceFileName, "r");

    while(fscanf (traceFile, "%s %x,%d", opt, &addr, &size) != EOF) {
//        printf("==== %d\n", addr);
        if(strcmp(opt, "I") == 0) {
            continue;
        }
        int setBits = getSet (addr, s, b);
        int tagBits = getTag (addr, s, b);
        if(isVerbose == 1) {
            printf("%s %x,%d ", opt, addr, size);
        }
        if(strcmp(opt, "S") == 0) {
            storeData (&cache, addr, size, setBits, tagBits, isVerbose);
        }
        if(strcmp(opt, "M") == 0) {
            modifyData (&cache, addr, size, setBits, tagBits, isVerbose);
        }
        if(strcmp(opt, "L") == 0) {
            loadData (&cache, addr, size, setBits, tagBits, isVerbose);
        }
        if(isVerbose == 1) {
            printf("\n");
        }
    }

    printSummary(hits, misses, evictions);
    return 0;
}

int getSet (int addr, int s, int b) {
    // addr = t + s + b, we want to get s
    addr = addr >> b;
    int mask = (1 << s) - 1;
    return (addr & mask);
}

int getTag (int addr, int s, int b) {
    int mask = s + b;
    return (addr >> mask);
}

int findMinLruNumber(SimCache *simCache, int setBits) {
    int minIndex = 0;
    int minLru = MAGIC_LRU_NUMBER;
    for(int i = 0; i < simCache->lineNumber; i++) {
        if(simCache->sets[setBits].lines[i].lruNumber < minLru) {
            minIndex = i;
            minLru = simCache->sets[setBits].lines[i].lruNumber;
        }
    }
    return minIndex;
}

int isMiss (SimCache *simCache, int setBits, int tagBits) {
    int isMiss = 1;
    for(int i = 0; i < simCache->lineNumber; i++) {
        if(simCache->sets[setBits].lines[i].valid == 1 && simCache->sets[setBits].lines[i].tag == tagBits) {
            printf ("successful\n");
            isMiss = 0;
            updateLruNumber (simCache, setBits, i);
        }
    }
    return isMiss;
}

void updateLruNumber(SimCache *simCache, int setBits, int hitIndex) {
    simCache->sets[setBits].lines[hitIndex].lruNumber = MAGIC_LRU_NUMBER;
    for(int i = 0; i < simCache->lineNumber; i++) {
        if(i != hitIndex) {
            simCache->sets[setBits].lines[i].lruNumber--;
        }
    }
}

void loadData (SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose) {
    if(isMiss(simCache, setBits, tagBits) == 1) {
        // miss
        misses++;
        if(isVerbose == 1) {
            printf("miss ");
        }
        if(updateCache(simCache, setBits, tagBits) == 1) {
            // this set is full, need a sacrifice line
            evictions++;
            if(isVerbose == 1) {
                printf("eviction ");
            }
        }
    } else {
        // hit
        hits++;
        if(isVerbose == 1) {
            printf("hit ");
        }
    }
}

int updateCache (SimCache *simCache, int setBits, int tagBits) {
    int isFull = 1, i = 0;
    for(i = 0; i < simCache->lineNumber; i++) {
        if(simCache->sets[setBits].lines[i].valid == 0) {
            isFull = 0; // not full
            break;
        }
    }
//    if(i >= simCache->lineNumber) {
//        printf("\nover the size, erro\n");
//        exit(0);
//    }

    if(isFull == 0) {
        simCache->sets[setBits].lines[i].valid = 1;
        simCache->sets[setBits].lines[i].tag = tagBits;
//        printf("simCache->sets[setBits].lines[i].tag = %d" , simCache->sets[setBits].lines[i].tag);
        updateLruNumber (simCache, setBits, i);
    } else {
        int evictionIndex = findMinLruNumber (simCache, setBits);
        // !!!!!!!! here use evictionIndex, not i, spend my one hour to find this error!!!!
        simCache->sets[setBits].lines[evictionIndex].valid = 1;
        simCache->sets[setBits].lines[evictionIndex].tag = tagBits;
//        printf("simCache->sets[setBits].lines[i].tag = %d" , simCache->sets[setBits].lines[i].tag);
        updateLruNumber (simCache, setBits, evictionIndex);
    }
    return isFull;
}

void storeData (SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose) {
    loadData (simCache, addr, size, setBits, tagBits, isVerbose);
}

void modifyData (SimCache *simCache, int addr, int size, int setBits, int tagBits, int isVerbose) {
    loadData (simCache, addr, size, setBits, tagBits, isVerbose);
    storeData (simCache, addr, size, setBits, tagBits, isVerbose);
}

void initSimCache (int s, int E, int b, SimCache *simCache) {
    if(s < 0) {
        printf("Invalid cache sets number\n");
        exit(0);
    }
    simCache->setNumber = 2 << s;
    simCache->lineNumber = E;
    simCache->sets = (Set *) malloc(sizeof(Set) * simCache->setNumber);
    if(!simCache->sets) {
        printf("Set memory malloc error\n");
        exit(0);
    }

    int i = 0, j = 0;
    for(i = 0; i < simCache->setNumber; i++) {
        simCache->sets[i].lines = (Line *)malloc(E * sizeof(Line));
        if(!simCache->sets[i].lines) {
            printf("Line memory malloc error\n");
            exit(0);
        }
        for(j = 0; j < E; j++) {
            simCache->sets[i].lines[j].valid = 0;
//            simCache->sets[i].lines[j].tag = 0;
            simCache->sets[i].lines[j].lruNumber = 0;
        }
    }
}

void checkOptArg (char *curOptArg) {
    if(curOptArg[0] == '-') {
        printf("./csim : Missing required command line arguments\n");
        printHelpMen ();
        exit(0);
    }
}

int getOpt (int argc, char **argv, int *s, int *E, int *b, char *traceFileNmae, int *isVerbose) {
    int c = 0;
    while((c = getopt (argc, argv, "hvs:E:b:t:")) != -1) {
        switch (c) {
        case 'v':
            *isVerbose = 1;
            break;
        case 's':
            checkOptArg (optarg);
            *s = atoi(optarg);
            break;
        case 'E' :
            checkOptArg (optarg);
            *E = atoi(optarg);
            break;
        case 'b':
            checkOptArg (optarg);
            *b = atoi(optarg);
            break;
        case 't':
            checkOptArg (optarg);
            strcpy(traceFileNmae, optarg);
            break;
        case 'h':
        default:
            printHelpMen ();
            break;
        }
    }
    return 1;
}

void printHelpMen () {
    printf("Usage : ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file> \n");
    printf("Options : \n");
    printf("-h              Print this help message.\n");
    printf("-v              Optional verbose flag.\n");
    printf("-s <num>        Number of set index bits.\n");
    printf("-E <num>        Number of lines per set.\n");
    printf("-b <num>        Number of block offset bits.\n");
    printf("-t <file>       Trace file.\n\n\n");
    printf("Examples : \n");
    printf("linux> ./csim -s 4 -E 1 -b 4 -t trace/yi.trace\n");
}



















