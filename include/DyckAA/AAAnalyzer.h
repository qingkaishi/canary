/* 
 * File:   AAAnalyzer.h
 *
 * Created on November 9, 2013, 3:45 PM
 * 
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef AAANALYZER_H
#define	AAANALYZER_H

#include "DyckAA/EdgeLabel.h"
#include "DyckAA/DyckAliasAnalysis.h"
#include "DyckAA/ProgressBar.h"
#include <map>
#include <unordered_map>

using namespace std;

class DyckAliasAnalysis;

typedef struct FunctionTypeNode {
	FunctionType * type;
	FunctionTypeNode * root;
	set<Function *> compatibleFuncs;
} FunctionTypeNode;

class AAAnalyzer {
private:
	Module* module;
	DyckAliasAnalysis* aa;
	DyckGraph* dgraph;
	DyckCallGraph* callgraph;

private:
	map<Type*, FunctionTypeNode*> functionTyNodeMap;
	set<FunctionTypeNode *> tyroots;

	DyckAA::ProgressBar PB;

public:
	AAAnalyzer(Module* m, DyckAliasAnalysis* a, DyckGraph* d, DyckCallGraph* cg);
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
	void handle_inst(Instruction *inst, DyckCallGraphNode * parent);
	void handle_instrinsic(Instruction *inst);
	void handle_extract_insert_value_inst(Value* aggValue, Type* aggTy, ArrayRef<unsigned>& indices, Value* insertedOrExtractedValue);
	void handle_extract_insert_elmt_inst(Value* vec, Value* elmt);
	void handle_invoke_call_inst(Instruction * ret, Value* cv, vector<Value*>* args, DyckCallGraphNode * parent);
	void handle_lib_invoke_call_inst(Value* ret, Function* f, vector<Value*>* args, DyckCallGraphNode* parent);

private:
	bool handle_pointer_function_calls(DyckCallGraphNode* caller, int counter);
	void handle_common_function_call(Call* c, DyckCallGraphNode* caller, DyckCallGraphNode* callee);

private:
	int isCompatible(FunctionType * t1, FunctionType * t2);
	set<Function*>* getCompatibleFunctions(FunctionType * fty);

	FunctionTypeNode* initFunctionGroup(FunctionType* fty);
	void initFunctionGroups();
	void destroyFunctionGroups();
	void combineFunctionGroups(FunctionType * ft1, FunctionType* ft2);

private:
	DyckVertex* addField(DyckVertex* val, long fieldIndex, DyckVertex* field);
	DyckVertex* addPtrTo(DyckVertex* address, DyckVertex* val);
	DyckVertex* makeAlias(DyckVertex* x, DyckVertex* y);
	void makeContentAlias(DyckVertex* x, DyckVertex* y);

	DyckVertex* handle_gep(GEPOperator* gep);
	DyckVertex* wrapValue(Value * v);
};

#endif	/* AAANALYZER_H */

