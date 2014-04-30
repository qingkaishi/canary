/* 
 * File:   Cache.h
 * Author: qingkaishi
 *
 * Created on April 29, 2014, 5:32 PM
 */

#ifndef CACHE_H
#define	CACHE_H

#include <stdio.h>

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

    CacheNode* __cache_lines;

public:

    Cache() : __head(0), __size_limit(20), __size(0) {
        __cache_lines = new CacheNode[__size_limit];
    }

    Cache(size_t size) : __head(0), __size(0) {
        __size_limit = size < 20 ? 20 : size;
        __cache_lines = new CacheNode[__size_limit];
    }

    ~Cache() {
        delete [] __cache_lines;
    }

    void add(size_t address, size_t value) {
        if (__size == __size_limit) {
            int idx = __head % __size_limit;
            CacheNode& cn = __cache_lines[idx];
            
            cn.address = address;
            cn.value = value;
        } else {
            CacheNode& cn = __cache_lines[__head + 1];
            cn.address = address;
            cn.value = value;
            
            __head++;
            __size++;
        }
    }

    bool query(size_t address, size_t value) {
        for (int idx = 0; idx < __size; idx++) {
            int delta = __head - idx;
            CacheNode& c = (delta >= 0) ? __cache_lines[delta] : __cache_lines[__size_limit + delta];

            if (c.address == address && c.value == value) {
                return true;
            }
        }

        return false;
    }

};

#endif	/* CACHE_H */

