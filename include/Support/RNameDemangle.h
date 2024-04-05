

#ifndef SUPPORT_RNAMEDEMANGLE_H
#define SUPPORT_RNAMEDEMANGLE_H
#include <string>

class RNameDemangle{
public:
    static std::string legacyDemangle(std::string s);
};

#endif