/* 
 * File:   Log.h
 * Author: qingkaishi
 *
 * Created on April 21, 2014, 9:37 PM
 */

#ifndef LOG_H
#define	LOG_H

#include <vector>

#define MAX_LOG_LEN 50000

template<typename T> class Item {
public:
    T t;
    unsigned counter;
};

template<typename T> class Log {
public:
    std::vector<Item<T>* > __log;
    size_t __size;
    bool __complete;

    enum DUMP_TYPE {
        PLAIN, // do not output idx
        LOP, // output t and idx
        DLOP // compress idx again
    };

private:

    void __plain_dump(FILE * fout) {
#ifdef DEBUG
        printf("using __plain_dump\n");
#endif

        unsigned size = 0;
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            size = size + item->counter;
        }

#ifdef LDEBUG
        fprintf(fout, "Type: __plain_dump (%d). Size: %u.\n", __complete, size);
#else
        fwrite(&size, sizeof (unsigned), 1, fout); // size
#endif
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            for (unsigned j = 0; j < item->counter; j++) {
#ifdef LDEBUG
                fprintf(fout, "%ld; ", item->t);
#else
                fwrite(&item->t, sizeof (T), 1, fout);
#endif
            }
        }
#ifdef LDEBUG
        fprintf(fout, "\n");
#endif
    }

    void __plain_load(FILE* fin) {
        unsigned size;
        fread(&size, sizeof (unsigned), 1, fin); // size
        T t[size];
        fread(t, sizeof (T), size, fin);

        for (unsigned i = 0; i < size; i++) {
            this->logValue(t[i]);
        }
    }

    void __lop_dump(FILE * fout) {
#ifdef DEBUG
        printf("using __lop_dump\n");
#endif

        unsigned size = __log.size(); // size
#ifdef LDEBUG
        fprintf(fout, "Type: __lop_dump (%d). Size: %u.\n", __complete, size);
#else
        fwrite(&size, sizeof (unsigned), 1, fout);
#endif

        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
#ifdef LDEBUG
            fprintf(fout, "(%ld, %u); ", item->t, item->counter);
#else
            fwrite(item, sizeof (Item<T>), 1, fout);
#endif
        }

#ifdef LDEBUG
        fprintf(fout, "\n");
#endif
    }

    void __lop_load(FILE* fin) {
        fread(&__size, sizeof (unsigned), 1, fin); // size
        Item<T> t[__size];
        fread(t, sizeof (Item<T>), __size, fin);

        for (unsigned i = 0; i < __size; i++) {
            Item<T> * elmt = new Item<T>;
            memcpy(elmt, &t[i], sizeof (Item<T>));
            this->__log.push_back(elmt);
        }
    }

    void __dlop_dump(FILE * fout) {
#ifdef DEBUG
        printf("using __dlop_dump\n");
#endif

        size_t s3 = 0;
        unsigned lastOne = -1;
        for (unsigned i = 0; i < __log.size(); i++) {
            if (__log[i]->counter != lastOne) {
                s3 = s3 + 2;
                lastOne = __log[i]->counter;
            }
        }

        unsigned size = __log.size();
#ifdef LDEBUG
        fprintf(fout, "Type: __dlop_dump (%d). Size: %u. CounterSize: %u\n", __complete, size, s3);
#else
        fwrite(&size, sizeof (unsigned), 1, fout); // size
        fwrite(&s3, sizeof (unsigned), 1, fout); // counter size
#endif
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
#ifdef LDEBUG
            fprintf(fout, "%ld; ", item->t);
#else
            fwrite(&item->t, sizeof (T), 1, fout);
#endif
        }
#ifdef LDEBUG
        fprintf(fout, "\n");
#endif

        unsigned counter = 1;
        lastOne = __log[0]->counter;
        for (unsigned i = 1; i < __log.size(); i++) {
            if (__log[i]->counter != lastOne) {
#ifdef LDEBUG
                fprintf(fout, "(%u, %u); ", lastOne, counter);
#else
                fwrite(&lastOne, sizeof (unsigned), 1, fout);
                fwrite(&counter, sizeof (unsigned), 1, fout);
#endif
                lastOne = __log[i]->counter;
                counter = 1;
            } else {
                counter++;
            }
        }
#ifdef LDEBUG
        fprintf(fout, "(%u, %u); \n", lastOne, counter);
#else
        fwrite(&lastOne, sizeof (unsigned), 1, fout);
        fwrite(&counter, sizeof (unsigned), 1, fout);
#endif
    }

    void __dlop_load(FILE* fin) {
        unsigned size = 0;
        unsigned counter_size = 0;
        fread(&size, sizeof (unsigned), 1, fin); // size
        fread(&counter_size, sizeof (unsigned), 1, fin); // counter size

        T t[size];
        fread(t, sizeof (T), size, fin);

        unsigned counter[counter_size];
        fread(counter, sizeof (unsigned), counter_size, fin);

        unsigned real_counters[size];
        unsigned idx = 0;
        for (unsigned i = 0; i < counter_size; i += 2) {
            for (unsigned j = 0; j < counter[i + 1]; j++) {
                if (idx >= size) {
                    printf("[ERROR] load error!\n")
                    exit(1);
                }
                real_counters[idx++] = counter[i];
            }
        }


        for (unsigned i = 0; i < size; i++) {
            Item<T>* elmt = new Item<T>;
            elmt->t = t[i];
            elmt->counter = real_counters[i];

            __log.push_back(elmt);
        }

        __size = size;
    }

public:

    Log() : __size(0), __complete(true) {
    }

    Log(size_t size) : __size(0), __complete(true) {
        __log.resize(size);
    }

    virtual ~Log() {
        for (unsigned i = 0; i < __log.size(); i++) {
            delete __log[i];
        }
    }

    virtual void load(FILE* fin) {
        DUMP_TYPE type;
        fread(&type, sizeof (DUMP_TYPE), 1, fin); // type
        fread(&__complete, sizeof (bool), 1, fin); // __complete?
        if (!__complete) {
            printf("[ERROR] incomplete log is not supported in the version!\n");
        }
        switch (type) {
            case PLAIN:
                __plain_load(fin);
                break;
            case LOP:
                __lop_load(fin);
                break;
            case DLOP:
                __dlop_load(fin);
                break;
        }
    }

    virtual DUMP_TYPE evaluate() {
        size_t s1 = 0;
        for (unsigned i = 0; i < __log.size(); i++) {
            s1 = s1 + __log[i]->counter;
        }
        s1 = s1 * sizeof (T);

        size_t s2 = __log.size() * (sizeof (T) + sizeof (unsigned));

        size_t s3 = __log.size() * sizeof (T);
        unsigned lastOne = -1;
        for (unsigned i = 0; i < __log.size(); i++) {
            if (__log[i]->counter != lastOne) {
                s3 = s3 + 2 * sizeof (unsigned);
                lastOne = __log[i]->counter;
            }
        }

        if (s1 <= s2 && s1 <= s3) return PLAIN;
        if (s2 <= s1 && s2 <= s3) return LOP;
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

    virtual void dumpWithValueUnsignedMap(FILE* fout, boost::unordered_map<T, unsigned>& map) {
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            item->t = map[item->t];
        }

        dump(fout);
    }

    virtual void logValue(T val) = 0;
};

template<typename T> class LastOnePredictorLog : public Log<T> {
private:

    void lastOnePredictorStore(T val) {
        size_t currentIdx = -1;
        int previousIdx = Log<T>::__size - 1;
        if (Log<T>::__size == MAX_LOG_LEN) {
            Log<T>::__complete = false;
            currentIdx = 0;
            Log<T>::__size = 0;
        } else {
            currentIdx = Log<T>::__size;
        }
        Item<T>* x = 0;
        if (previousIdx >= 0 && (x = Log<T>::__log[previousIdx]) && memcmp(&(x->t), &val, sizeof (T)) == 0) {
            x->counter = x->counter + 1;
        } else if (Log<T>::__log.size() == MAX_LOG_LEN) {
            Item<T> * vI = Log<T>::__log[currentIdx];
            vI->counter = 1;
            memcpy(&(vI->t), &val, sizeof (T));
            Log<T>::__size++;
        } else {
            Item<T> * vI = new Item<T>;
            memcpy(&(vI->t), &val, sizeof (T));
            vI->counter = 1;
            Log<T>::__log.push_back(vI);
            Log<T>::__size++;
        }
    }

public:

    LastOnePredictorLog() : Log<T>() {
    }

    LastOnePredictorLog(size_t size) : Log<T>(size) {
    }

    virtual void logValue(T val) {
        lastOnePredictorStore(val);
    }

};

class VLastOnePredictorLog : public Log<size_t> {
private:

    void vLastOnePredictorStore(size_t val) {
        size_t currentIdx = -1;
        int previousIdx = __size - 1;
        if (__size == MAX_LOG_LEN) {
            __complete = false;
            currentIdx = 0;
            __size = 0;
        } else {
            currentIdx = __size;
        }

        Item<size_t> * x = 0;
        if (previousIdx >= 0 && (x = __log[previousIdx]) && x->counter + x->t == val) {
            x->counter = x->counter + 1;
        } else if (__log.size() == MAX_LOG_LEN) {
            Item<size_t> * vI = __log[currentIdx];
            vI->counter = 1;
            vI->t = val;

            __size++;
        } else {
            Item<size_t> * vI = new Item<size_t>;
            vI->t = val;
            vI->counter = 1;
            __log.push_back(vI);

            __size++;
        }
    }

public:

    VLastOnePredictorLog() : Log<size_t>() {
    }

    VLastOnePredictorLog(size_t size) : Log<size_t>(size) {
    }

    virtual void logValue(size_t val) {
        vLastOnePredictorStore(val);
    }

};

#define CANARY_THREADS_MAX 32

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

typedef LastOnePredictorLog<pthread_t> g_llog_t;

typedef LastOnePredictorLog<pthread_t> g_mlog_t;

typedef LastOnePredictorLog<pthread_t> g_flog_t;

typedef struct {
    void* address;
    size_t range;
} mem_t;

typedef struct {
    std::vector<mem_t *> ADDRESS_LOG;
    VLastOnePredictorLog BIRTHDAY_LOG;
} l_addmap_t;

#endif	/* LOG_H */

