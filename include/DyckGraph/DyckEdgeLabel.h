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

#ifndef DYCKGRAPH_DYCKEDGELABEL_H
#define DYCKGRAPH_DYCKEDGELABEL_H

#include <string>
#include <map>

class DyckEdgeLabel {
public:
    enum LABEL_TY {
        DEREF_TYPE, OFFSET_TYPE, INDEX_TYPE
    };
private:
    std::string desc;
public:
    virtual std::string &getEdgeLabelDescription() { return desc; }

    virtual bool isLabelTy(LABEL_TY type) { return false; }

    virtual ~DyckEdgeLabel() = default;
};

class DerefEdgeLabel : public DyckEdgeLabel {
public:
    DerefEdgeLabel() {
        std::string &desc = DyckEdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("D");
    }

    bool isLabelTy(LABEL_TY type) override { return type == DyckEdgeLabel::DEREF_TYPE; }
};

class PointerOffsetEdgeLabel : public DyckEdgeLabel {
private:
    long offset_bytes;
public:
    explicit PointerOffsetEdgeLabel(long bytes) : offset_bytes(bytes) {
        std::string &desc = DyckEdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("@");

        char temp[1024];
        sprintf(temp, "%ld", bytes);
        desc.append(temp);
    }

    long getOffsetBytes() const { return offset_bytes; }

    bool isLabelTy(LABEL_TY type) override { return type == DyckEdgeLabel::OFFSET_TYPE; }
};

class FieldIndexEdgeLabel : public DyckEdgeLabel {
private:
    long field_index;
public:
    explicit FieldIndexEdgeLabel(long idx) : field_index(idx) {
        std::string &desc = DyckEdgeLabel::getEdgeLabelDescription();
        desc.clear();
        desc.append("#");

        char temp[1024];
        sprintf(temp, "%ld", idx);
        desc.append(temp);
    }

    long getFieldIndex() const { return field_index; }

    bool isLabelTy(LABEL_TY type) override { return type == DyckEdgeLabel::INDEX_TYPE; }
};

#endif // DYCKGRAPH_DYCKEDGELABEL_H

