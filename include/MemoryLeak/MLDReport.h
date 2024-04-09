#ifndef MEMORYLEAK_MLDREPORT_H
#define MEMORYLEAK_MLDREPORT_H

#include "llvm/IR/Value.h"
#include "llvm/Support/raw_ostream.h"
class MLDReport {
  public:
    enum ReportType { NeverFree, PartialFree, SafeFree };

    llvm::Value *Value;
    ReportType Rty;

    MLDReport(llvm::Value *Value, ReportType Rty)
        : Value(Value),
          Rty(Rty) {}

    std::string toString() {
        std::string s;
        std::string srty;
        switch (Rty) {
        case NeverFree:
            srty = "\033[31mNeverFree\033[0m";
            break;
        case PartialFree:
            srty = "\033[34mPartialFree\033[0m";
            break;
        case SafeFree:
            srty = "\033[32mSafeFree\033[0m";
            break;
        }
        llvm::raw_string_ostream s_stream(s);
        s_stream << *Value << " is " << srty << ".";
        return s;
    }
};

#endif