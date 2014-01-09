/* 
 * File:   AAAnalyzer.h
 * Author: jack
 *
 * Created on November 9, 2013, 3:45 PM
 */

#ifndef AAANALYZER_H
#define	AAANALYZER_H

#define DEREF_LABEL -1

#include "DyckGraph.h"
#include "FunctionWrapper.h"
#include <map>
#include <tr1/unordered_map>

using namespace std;

typedef struct FunctionTypeNode {
    FunctionType * type;
    FunctionTypeNode * root;
    set<Function *> compatibleFuncs;
} FunctionTypeNode;

class AAAnalyzer {
private:
    Module* module;
    AliasAnalysis* aa;
    DyckGraph* dgraph;

private:
    map<Type*, FunctionTypeNode*> functionTyNodeMap;
    set<FunctionTypeNode *> tyroots;

private:
    map<Function*, FunctionWrapper *> wrapped_functions_map;
    set<FunctionWrapper*> wrapped_functions;
    vector<CallInst*> tempCalls;

public:
    AAAnalyzer(Module* m, AliasAnalysis* a, DyckGraph* d);
    ~AAAnalyzer();

    void start_intra_procedure_analysis();
    void end_intra_procedure_analysis();

    void start_inter_procedure_analysis();
    void end_inter_procedure_analysis();

    bool intra_procedure_analysis();
    bool inter_procedure_analysis();

    void getValuesEscapedFromThreadCreate(set<Value*>* ret);

private:
    void handle_inst(Instruction *inst, FunctionWrapper * parent);
    bool handle_functions(FunctionWrapper* caller);
    void handle_instrinsic(Instruction *inst);
    void handle_common_function_call(Value* ci, FunctionWrapper* caller, FunctionWrapper* callee);
    void storeCandidateFunctions(FunctionWrapper* parent, Value * call, Value * cv);

private:
    bool isCompatible(FunctionType * t1, FunctionType * t2);
    void initFunctionGroups();
    void destroyFunctionGroups();

private:
    DyckVertex* addField(DyckVertex* val, int fieldIndex, DyckVertex* field);
    DyckVertex* addPtrTo(DyckVertex* address, DyckVertex* val);
    void makeAlias(DyckVertex* x, DyckVertex* y);

    DyckVertex* handle_gep(GEPOperator* gep);
    DyckVertex* wrapValue(Value * v);
};

#endif	/* AAANALYZER_H */

