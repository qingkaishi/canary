/*
 * File:   Log.h
 * 
 * Created on April 21, 2014, 9:37 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef LOG_H
#define	LOG_H

#include <vector>
#include <algorithm> 

#define BUFF_NM 100
#define BUFF_SZ 100

template<typename T> class Item {
public:
    T t;
    unsigned counter;

    Item() : counter(1) {
    }

    Item(T tv, unsigned cv) : t(tv), counter(cv) {
    }
};

template<typename T> class Log {
public:
    std::vector<std::vector<Item<T> >* > __log;
    size_t __size;
    bool __complete;

    std::vector<Item<T> >* __buff;
    size_t __buff_ptr;

    enum DUMP_TYPE {
        PLAIN, // do not output idx
        LOP, // output t and idx
        DLOP // compress idx again
    };

public:

    Log() : __size(0), __complete(true), __buff_ptr(0) {
        Item<T> item;
        __buff = new std::vector<Item<T> >(BUFF_SZ, item);
        //__buff->assign(BUFF_SZ, item);
    }

    virtual ~Log() {
        for (unsigned i = 0; i < __log.size(); i++) {
            delete __log[i];
        }
    }

private:

    void __plain_dump(FILE * fout) {
    }

    void __lop_dump(FILE * fout) {
    }

    void __dlop_dump(FILE * fout) {
    }

public:

    virtual DUMP_TYPE evaluate() {
        return DLOP;
    }

    virtual unsigned size() {
        return __log.size();
    }

    virtual void default_dump(FILE * fout) {
        __lop_dump(fout);
    }

    virtual void dump(FILE * fout) {
        DUMP_TYPE t = evaluate();
#ifndef LDEBUG
        fwrite(&t, sizeof (DUMP_TYPE), 1, fout); // type
        fwrite(&__complete, sizeof (bool), 1, fout); // __complete?
#endif
        switch (t) {
            case PLAIN:
                __plain_dump(fout);
                break;
            case LOP:
                __lop_dump(fout);
                break;
            case DLOP:
                __dlop_dump(fout);
                break;
        }
    }

    virtual void logValue(T val) = 0;
};

template<typename T> class LastOnePredictorLog : public Log<T> {
private:

    void lastOnePredictorStore(T val) {
        Item<T>& x = Log<T>::__buff->at(Log<T>::__buff_ptr);
        if (x.t == val) {
            x.counter = x.counter + 1;
        } else if (Log<T>::__buff_ptr < BUFF_SZ - 1) {
            Item<T>& n = Log<T>::__buff->at(++Log<T>::__buff_ptr);

            n.t = val;
            n.counter = 1;
        } else {
            // add buffer to __log
            if (Log<T>::__size == BUFF_NM) {
                Log<T>::__complete = false;
                Log<T>::__size = 0;
            }

            Log<T>::__buff_ptr = 0;

            //__log//insert __buff from currentIdx to currentIdx + 100
            if (Log<T>::__complete) {
                // new buffer
                Item<T> item(val, 1);
                Log<T>::__buff = new std::vector<Item<T> >(BUFF_SZ, item);
                //Log<T>::__buff->assign(BUFF_SZ, item);

                Log<T>::__log.push_back(Log<T>::__buff);
            } else {
                Log<T>::__buff = Log<T>::__log[0];
                Item<T>& item = Log<T>::__buff->at(Log<T>::__buff_ptr);

                item.counter = 1;
                item.t = val;
            }
            Log<T>::__size++;
        }
    }

public:

    LastOnePredictorLog() : Log<T>() {
    }

    virtual void logValue(T val) {
        lastOnePredictorStore(val);
    }

};

class VLastOnePredictorLog : public Log<size_t> {
private:

    void vLastOnePredictorStore(size_t val) {
        Item<size_t>& x = __buff->at(__buff_ptr);
        if (x.counter + x.t == val) {
            x.counter = x.counter + 1;
        } else if (__buff_ptr < BUFF_SZ - 1) {
            Item<size_t>& n = __buff->at(++__buff_ptr);

            n.t = val;
            n.counter = 1;
        } else {
            // add buffer to __log
            if (__size == BUFF_NM) {
                __complete = false;
                __size = 0;
            }

            __buff_ptr = 0;

            //__log//insert __buff from currentIdx to currentIdx + 100
            if (__complete) {
                // new buffer
                Item<size_t> item(val, 1);
                __buff = new std::vector<Item<size_t> >(BUFF_SZ, item);
                //__buff->assign(BUFF_SZ, item);

                __log.push_back(__buff);
            } else {
                __buff = __log[0];
                Item<size_t>& item = __buff->at(__buff_ptr);

                item.counter = 1;
                item.t = val;
            }
            __size++;
        }
    }

public:

    VLastOnePredictorLog() : Log<size_t>() {
    }

    virtual void logValue(size_t val) {
        vLastOnePredictorStore(val);
    }

};

/*
 * Define log types
 * l_ means local log types, each thread has a log
 * g_ means global log types, each shared variable has a log
 * r  -> read 
 * lr -> local read
 * w  -> write
 * l  -> lock
 * m  -> mutex init
 * f  -> fork
 */
typedef struct {
    // <long/void *> sizeof pointer usually equals to sizeof long
    LastOnePredictorLog<long> VAL_LOG;
    // <int>, -1 is the place holder, means the read operation reads
    // the local value
    LastOnePredictorLog<int> VER_LOG;
} l_rlog_t;

typedef VLastOnePredictorLog l_wlog_t;

typedef LastOnePredictorLog<long> l_lrlog_t;

typedef LastOnePredictorLog<unsigned> g_llog_t;

typedef LastOnePredictorLog<unsigned> g_mlog_t;

typedef LastOnePredictorLog<unsigned> g_flog_t;

typedef struct {
    void* address;
    size_t range;
    int type;
} mem_t;

typedef struct {
    std::vector<mem_t> adds;
    bool stack_tag;
} l_addmap_t;

#endif	/* LOG_H */

