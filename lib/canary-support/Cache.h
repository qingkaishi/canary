/* 
 * File:   Cache.h
 * Author: qingkaishi
 *
 * Created on April 29, 2014, 5:32 PM
 */

#ifndef CACHE_H
#define	CACHE_H

#include <stdio.h>

#define DEFAULT_CACHE_SIZE 100

class CacheNode {
public:
    size_t address;
    size_t value;

    CacheNode() {
    }

    CacheNode(size_t a, size_t v) : address(a), value(v) {
    }
};

class Cache {
private:
    int __head;
    int __size_limit;
    int __size;

    boost::unordered_map<size_t, int> __map;
    CacheNode* __cache_lines;

public:

    Cache() : __head(0), __size_limit(DEFAULT_CACHE_SIZE), __size(0) {
        __cache_lines = new CacheNode[__size_limit];
    }

    Cache(size_t size) : __head(0), __size(0) {
        __size_limit = size < DEFAULT_CACHE_SIZE ? DEFAULT_CACHE_SIZE : size;
        __cache_lines = new CacheNode[__size_limit];
    }

    ~Cache() {
        delete [] __cache_lines;
    }

    void add(size_t address, size_t value) {
        if(__map.count(address)){
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

    bool query(size_t address, size_t value) {
        return __map.count(address) && __cache_lines[__map[address]] == value;
    }

};

#endif	/* CACHE_H */

