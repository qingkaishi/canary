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
    unsigned idx;
};

template<typename T> class Log {
public:
    std::vector<Item<T>* > __log;
    bool __recording;

public:

    Log() {
        __recording = true;
    }

    Log(size_t size) {
        __log.resize(size);
        __recording = true;
    }

    ~Log() {
        for (unsigned i = 0; i < __log.size(); i++) {
            delete __log[i];
        }
    }
    
    unsigned size(){
        return __log.size();
    }

    void dump(const char* logfile, const char * mode) {
        FILE * fout = fopen(logfile, mode);

        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            fwrite(item, sizeof (Item<T>), 1, fout);
        }

        fclose(fout);
    }

    void dumpWithUnsigned(const char* logfile, const char * mode, unsigned u) {
        FILE * fout = fopen(logfile, mode);

        fwrite(&u, sizeof (unsigned), 1, fout);
        for (unsigned i = 0; i < __log.size(); i++) {
            Item<T> * item = __log[i];
            fwrite(item, sizeof (Item<T>), 1, fout);
        }

        fclose(fout);
    }

    virtual void logValue(T& val) = 0;
};

template<typename T> class LastOnePredictorLog : public Log<T> {
private:

    void lastOnePredictorStore(T& val) {
        size_t currentIdx = Log<T>::__log.size();
        if (currentIdx >= MAX_LOG_LEN) {
            if (Log<T>::__recording)
                printf("Log is too long to record! Stop recording!\n");
            Log<T>::__recording = false;
            return;
        }

        Item<T>* x = Log<T>::__log[currentIdx - 1];
        if (currentIdx > 0 && memcmp(&(x->t), &val, sizeof (T)) == 0) {
            x->idx = x->idx + 1;
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

    virtual void logValue(T& val) {
        lastOnePredictorStore(val);
    }

};

class VLastOnePredictorLog : public Log<unsigned> {
private:

    void vLastOnePredictorStore(unsigned& val) {
        size_t log_len = __log.size();
        if (log_len >= MAX_LOG_LEN) {
            if (__recording)
                printf("Log is too long to record! Stop recording!\n");
            __recording = false;
            return;
        }

        Item<unsigned> * x = __log[log_len - 1];
        if (log_len > 0 && x->idx + x->t == val) {
            x->idx = x->idx + 1;
        } else {
            Item<unsigned> * vI = new Item<unsigned>;
            vI->t = val;
            vI->t = 1;
            __log.push_back(vI);
        }
    }

public:

    VLastOnePredictorLog() : Log<unsigned>() {
    }

    VLastOnePredictorLog(size_t size) : Log<unsigned>(size) {
    }

    virtual void logValue(unsigned& val) {
        vLastOnePredictorStore(val);
    }
};

#endif	/* LOG_H */

