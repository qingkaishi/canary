/*
 * Developed by Qingkai Shi
 * Copy Right by Prism Research Group, HKUST and State Key Lab for Novel Software Tech., Nanjing University.  
 */

#ifndef EDGELABEL_H
#define	EDGELABEL_H

#include <stdio.h>
#include <map>

class EdgeLabel {
public:
    enum LABEL_TY {DEREF_TYPE, OFFSET_TYPE, INDEX_TYPE};
private:
    std::string desc;
public:
    virtual std::string& getEdgeLabelDescription() { return desc; }
    virtual bool isLabelTy(LABEL_TY type){ return false; }
    virtual ~EdgeLabel(){}
};

class DerefEdgeLabel : public EdgeLabel {
public:
    DerefEdgeLabel() {
        std::string& desc = EdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("D");
    }
    virtual bool isLabelTy(LABEL_TY type) { return type == EdgeLabel::DEREF_TYPE;}
};

class PointerOffsetEdgeLabel : public EdgeLabel {
private:
    long offset_bytes;
public:
    PointerOffsetEdgeLabel(long bytes) : offset_bytes(bytes){
        std::string& desc = EdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("@");
        
        char temp[1024];
        sprintf(temp, "%ld", bytes);
        desc.append(temp);
    }

    long getOffsetBytes() { return offset_bytes; }
    
    virtual bool isLabelTy(LABEL_TY type) { return type == EdgeLabel::OFFSET_TYPE;}
};

class FieldIndexEdgeLabel : public EdgeLabel {
private:
    long field_index;
public:
    FieldIndexEdgeLabel(long idx) : field_index(idx){
        std::string& desc = EdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("#");
        
        char temp[1024];
        sprintf(temp, "%ld", idx);
        desc.append(temp);
    }

    long getFieldIndex() { return field_index; }
    
    virtual bool isLabelTy(LABEL_TY type) { return type == EdgeLabel::INDEX_TYPE;}
};

#endif	/* EDGELABEL_H */

