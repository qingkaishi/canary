#include "Log.h"

Log::Log() {
    __log = cvector_create(sizeof (Item));
    __recording = true;
}

Log::Log(size_t size) {
    __log = cvector_create(sizeof (Item), size);
    __recording = true;
}

////////////////////////////////////////////////////////////////////////////

LastOnePredictorLog::LastOnePredictorLog() : Log(){
}

LastOnePredictorLog::LastOnePredictorLog(size_t size) : Log(size){
}

LastOnePredictorLog::lastOnePredictorStore(T val) {
    size_t currentIdx = cvector_length(log);
    if (currentIdx >= MAX_LOG_LEN) {
        printf("Log is too long to record! Stop recording!\n");
        return;
    }

    Item* x = (Item*) cvector_at(log, currentIdx - 2);
    Item* y = (Item*) cvector_at(log, currentIdx - 1);
    if (currentIdx > 0 && x->t == val) {
        y->idx = y->idx + 1;
    } else {
        Item vI;
        vI.t = val;
        cvector_pushback(log, &vI);
        
        vI.idx = 1;
        cvector_pushback(log, &vI);
    }
}

LastOnePredictorLog::logValue(T val) {
    lastOnePredictorStore(val);
}

////////////////////////////////////////////////////////////////////////////


VLastOnePredictorLog::VLastOnePredictorLog() : Log<unsigned>(){
}

VLastOnePredictorLog::VLastOnePredictorLog(size_t size) : Log<unsigned>(size){
}

VLastOnePredictorLog::vLastOnePredictorStore(unsigned val) {
    size_t log_len = cvector_length(__log);
    if (log_len >= MAX_LOG_LEN) {
        printf("Log is too long to record! Stop recording!\n");
        return;
    }

    Item* x = (Item*) cvector_at(__log, log_len - 1);
    Item* y = (Item*) cvector_at(__log, log_len - 2);
    if (log_len > 0 && x->idx + y->t == val) {
        x->idx = x->idx + 1;
    } else {
        Item vI;
        vI.t = val;
        cvector_pushback(log, &vI);
        
        vI.idx = 1;
        cvector_pushback(log, &vI);
    }
}

VLastOnePredictorLog::logValue(unsigned val) {
    vLastOnePredictorStore(val);
}

