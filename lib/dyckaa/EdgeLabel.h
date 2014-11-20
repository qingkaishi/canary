/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef EDGELABEL_H
#define	EDGELABEL_H

#include <stdio.h>
#include <map>

class DyckDerefEdgeLabel;
class DyckPointerOffsetEdgeLabel;

class EdgeLabel {
public:
    enum LABEL_TY {DEREF_TYPE, OFFSET_TYPE};
public:
    virtual std::string& getEdgeLabelDescription();
    virtual bool isLabelTy(LABEL_TY type);
};

class DerefEdgeLabel : public EdgeLabel {
private:
    std::string desc;
public:
    DerefEdgeLabel() {
        desc.clear();
        desc.append("deref");
    }
    virtual std::string& getEdgeLabelDescription() { return desc;}
    virtual bool isLabelTy(LABEL_TY type) { return type == EdgeLabel::DEREF_TYPE;}
};

class PointerOffsetEdgeLabel : public EdgeLabel {
private:
    long offset_bytes;
    std::string desc;
    
public:
    PointerOffsetEdgeLabel(long bytes) : offset_bytes(bytes){
        desc.clear();
        desc.append("off");
        
        char temp[1024];
        sprintf(temp, "%ld", bytes);
        desc.append(temp);
    }
    
    virtual std::string& getEdgeLabelDescription() { return desc;}
    virtual bool isLabelTy(LABEL_TY type) { return type == EdgeLabel::OFFSET_TYPE;}
};

#endif	/* EDGELABEL_H */

