/* 
 * File:   Log.h
 * Author: qingkaishi
 *
 * Created on April 21, 2014, 9:37 PM
 */

#ifndef LOG_H
#define	LOG_H

#include "util/cvector.h"

#define MAX_LOG_LEN 50000

template<T> class Log {
private:
    cvector __log;
    bool __recording;

    typedef union {
        T t;
        unsigned idx;
    } Item;

public:
    Log();

    Log(size_t size);

    virtual void logValue(T val) = 0;
};

template<T> class LastOnePredictorLog : public Log<T> {
private:

    typedef union {
        T t;
        unsigned idx;
    } Item;

    void lastOnePredictorStore(T val);

public:
    LastOnePredictorLog();

    LastOnePredictorLog(size_t size);

    virtual void logValue(T val);

};

class VLastOnePredictorLog : public Log<unsigned> {
private:

    typedef union {
        unsigned t;
        unsigned idx;
    } Item;

    void vLastOnePredictorStore(unsigned val);

public:
    VLastOnePredictorLog();

    VLastOnePredictorLog(size_t size);

    virtual void logValue(unsigned val);
};

#endif	/* LOG_H */

