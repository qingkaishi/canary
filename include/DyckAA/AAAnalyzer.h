/*
 *  Canary features a fast unification-based alias analysis for C programs
 *  Copyright (C) 2021 Qingkai Shi <qingkaishi@gmail.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef AAANALYZER_H
#define AAANALYZER_H

#include "DyckAA/EdgeLabel.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/ProgressBar.h"
#include <map>
#include <unordered_map>

using namespace std;

class DyckAliasAnalysis;

typedef struct FunctionTypeNode {
    FunctionType *type;
    FunctionTypeNode *root;
    set<Function *> compatibleFuncs;
} FunctionTypeNode;

class AAAnalyzer {
private:
    Module *module;
    const DataLayout *dl;
    DyckAliasAnalysis *aa;
    DyckGraph *dgraph;
    DyckCallGraph *callgraph;

private:
    map<Type *, FunctionTypeNode *> functionTyNodeMap;
    set<FunctionTypeNode *> tyroots;

    DyckAA::ProgressBar PB;

public:
    AAAnalyzer(Module *m, DyckAliasAnalysis *a, DyckGraph *d, DyckCallGraph *cg);

    ~AAAnalyzer();

    void start_intra_procedure_analysis();

    void end_intra_procedure_analysis();

    void start_inter_procedure_analysis();

    void end_inter_procedure_analysis();

    void intra_procedure_analysis();

    void inter_procedure_analysis();

private:
    void printNoAliasedPointerCalls();

private:
    void handle_inst(Instruction *inst, DyckCallGraphNode *parent);

    void handle_instrinsic(Instruction *inst);

    void handle_extract_insert_value_inst(Value *aggValue, Type *aggTy, ArrayRef<unsigned> &indices,
                                          Value *insertedOrExtractedValue);

    void handle_extract_insert_elmt_inst(Value *vec, Value *elmt);

    void handle_invoke_call_inst(Instruction *ret, Value *cv, vector<Value *> *args, DyckCallGraphNode *parent);

    void handle_lib_invoke_call_inst(Value *ret, Function *f, vector<Value *> *args, DyckCallGraphNode *parent);

private:
    bool handle_pointer_function_calls(DyckCallGraphNode *caller, int counter);

    void handle_common_function_call(Call *c, DyckCallGraphNode *caller, DyckCallGraphNode *callee);

private:
    int isCompatible(FunctionType *t1, FunctionType *t2);

    set<Function *> *getCompatibleFunctions(FunctionType *fty);

    FunctionTypeNode *initFunctionGroup(FunctionType *fty);

    void initFunctionGroups();

    void destroyFunctionGroups();

    void combineFunctionGroups(FunctionType *ft1, FunctionType *ft2);

private:
    DyckVertex *addField(DyckVertex *val, long fieldIndex, DyckVertex *field);

    DyckVertex *addPtrTo(DyckVertex *address, DyckVertex *val);

    DyckVertex *makeAlias(DyckVertex *x, DyckVertex *y);

    void makeContentAlias(DyckVertex *x, DyckVertex *y);

    DyckVertex *handle_gep(GEPOperator *gep);

    DyckVertex *wrapValue(Value *v);
};

#endif    /* AAANALYZER_H */

