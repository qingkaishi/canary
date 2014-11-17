/* 
 * File:   Cache.h
 *
 * Created on April 29, 2014, 5:32 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.
 */

#ifndef CACHE_H
#define	CACHE_H

#include <stdio.h>

#define DEFAULT_CACHE_SIZE 200

class CacheNode {
public:
    void* address;
    long value;

    CacheNode() {
    }

    CacheNode(void* a, long v) : address(a), value(v) {
    }
};

class Cache {
private:
    int __head;
    int __size_limit;
    int __size;

    /*
     * For cache performance 
     */
    unsigned __hit;
    unsigned __miss;

    boost::unordered_map<void*, int> __map;
    CacheNode* __cache_lines;

public:

    Cache() : __head(0), __size_limit(DEFAULT_CACHE_SIZE), __size(0), __hit(0), __miss(0) {
        __cache_lines = new CacheNode[__size_limit];
    }

    Cache(size_t size) : __head(0), __size(0), __hit(0), __miss(0) {
        __size_limit = size < DEFAULT_CACHE_SIZE ? DEFAULT_CACHE_SIZE : size;
        __cache_lines = new CacheNode[__size_limit];
    }

    ~Cache() {
        delete [] __cache_lines;
    }

    void add(void* address, long value) {
        if (__map.count(address)) {
            // the same thread may write the same address
            CacheNode& cn = __cache_lines[__map[address]];
            cn.value = value;
            return;
        }

        if (__size == __size_limit) {
            int idx = __head % __size;
            CacheNode& cn = __cache_lines[idx];

            // erase the old one
            __map.erase(cn.address);

            cn.address = address;
            cn.value = value;

            // add the new one
            __map[address] = idx;

            __head = idx + 1;
        } else {
            CacheNode& cn = __cache_lines[__head];
            cn.address = address;
            cn.value = value;

            __map[address] = __head;

            __head++;
            __size++;
        }
    }

    bool query(void* address, long value) {
        if (__map.count(address) && __cache_lines[__map[address]].value == value) {
            __hit++;
            return true;
        } else {
            __miss++;
            return false;
        }
    }

    void clear() {
        __size = 0;
        __head = 0;
        __map.clear();
    }

    void info() {
        printf("[INFO] \tCache size: %d; ", __size_limit);
        printf("Cache hit rate: %.2lf%%(%d/%d)\n", __hit * 100 / (double) (__hit + __miss), __hit, __hit + __miss);
    }

};

#endif	/* CACHE_H */

