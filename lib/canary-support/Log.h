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
    bool __recording;

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

        DUMP_TYPE type = PLAIN;
        fwrite(&type, sizeof (DUMP_TYPE), 1, fout); // type
        fwrite(&size, sizeof (unsigned), 1, fout); // size
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            for (unsigned j = 0; j < item->counter; j++) {
                fwrite(&item->t, sizeof (T), 1, fout);
            }
        }
    }

    void __lop_dump(FILE * fout) {
#ifdef DEBUG
        printf("using __lop_dump\n");
#endif
        
        DUMP_TYPE type = LOP;
        fwrite(&type, sizeof (DUMP_TYPE), 1, fout); // type
        unsigned size = __log.size(); // size
        fwrite(&size, sizeof (unsigned), 1, fout);
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            fwrite(item, sizeof (Item<T>), 1, fout);
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

        DUMP_TYPE type = DLOP;
        fwrite(&type, sizeof (DUMP_TYPE), 1, fout); // type
        unsigned size = __log.size();
        fwrite(&size, sizeof (unsigned), 1, fout); // size
        fwrite(&s3, sizeof (unsigned), 1, fout); // counter size
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            fwrite(&item->t, sizeof (T), 1, fout);
        }

        unsigned counter = 1;
        lastOne = __log[0]->counter;
        for (unsigned i = 1; i < __log.size(); i++) {
            if (__log[i]->counter != lastOne) {
                fwrite(&lastOne, sizeof (unsigned), 1, fout);
                fwrite(&counter, sizeof (unsigned), 1, fout);
                lastOne = __log[i]->counter;
                counter = 1;
            } else {
                counter++;
            }
        }

        fwrite(&lastOne, sizeof (unsigned), 1, fout);
        fwrite(&counter, sizeof (unsigned), 1, fout);
    }

public:

    Log() {
        __recording = true;
    }

    Log(size_t size) {
        __log.resize(size);
        __recording = true;
    }

    virtual ~Log() {
        for (unsigned i = 0; i < __log.size(); i++) {
            delete __log[i];
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
        switch(t){
            case PLAIN: 
                __plain_dump(fout);
                break;
            case LOP: 
                __lop_dump(fout);
                break;
            case DLOP: 
                __dlop_dump(fout);
                break;
            default:
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

    virtual void dumpWithUnsigned(FILE* fout, unsigned u) {
        fwrite(&u, sizeof (unsigned), 1, fout);
        
        dump(fout);
    }

    virtual void logValue(T val) = 0;
};

template<typename T> class LastOnePredictorLog : public Log<T> {
private:

    void lastOnePredictorStore(T val) {
        size_t currentIdx = Log<T>::__log.size();
        if (currentIdx >= MAX_LOG_LEN) {
            if (Log<T>::__recording)
                printf("Log is too long to record! Stop recording!\n");
            Log<T>::__recording = false;
            return;
        }

        Item<T>* x = 0;
        if (currentIdx > 0 && (x = Log<T>::__log[currentIdx - 1]) && memcmp(&(x->t), &val, sizeof (T)) == 0) {
            x->counter = x->counter + 1;
        } else {
            Item<T> * vI = new Item<T>;
            memcpy(&(vI->t), &val, sizeof (T));
            vI->t = 1;
            Log<T>::__log.push_back(vI);
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
        size_t log_len = __log.size();
        if (log_len >= MAX_LOG_LEN) {
            if (__recording)
                printf("Log is too long to record! Stop recording!\n");
            __recording = false;
            return;
        }

        Item<size_t> * x = 0;
        if (log_len > 0 && (x = __log[log_len - 1]) && x->counter + x->t == val) {
            x->counter = x->counter + 1;
        } else {
            Item<size_t> * vI = new Item<size_t>;
            vI->t = val;
            vI->t = 1;
            __log.push_back(vI);
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
    /*
    virtual void dump(FILE * fout) {
        default_dump(fout);
    }

    virtual void dumpWithValueUnsignedMap(FILE* fout, boost::unordered_map<size_t, unsigned>& map) {
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<size_t> * item = __log[i];
            item->t = map[item->t];
        }
        
        default_dump(fout);
    }

    virtual void dumpWithUnsigned(FILE* fout, unsigned u) {
        fwrite(&u, sizeof (unsigned), 1, fout);
        
        default_dump(fout);
    }*/
};

#endif	/* LOG_H */

