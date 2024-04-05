
#include "Support/RNameDemangle.h"

using namespace std;

string RNameDemangle::legacyDemangle(string s) {
    string llvm(".llvm.");
    int i = s.find(llvm);
    if (i != string::npos) {
        bool all_hex = false;
        for (int j = i + llvm.size(); j < s.size(); j++) {
            all_hex &= (s.at(j) >= 'A' && s.at(j) <= 'F') || (s.at(j) >= '0' && s.at(j) <= '9') || (s.at(j) == '@');
        }
        if (all_hex) {
            s = s.substr(0, i);
        }
    }
    string inner;
    if (s.compare(0, 3, "_ZN", 0, 3) == 0) {
        inner = s.substr(3);
    }
    else if (s.compare(0, 2, "ZN", 0, 2) == 0) {
        inner = s.substr(2);
    }
    else if (s.compare(0, 4, "__ZN", 0, 4) == 0) {
        inner = s.substr(4);
    }
    else {
        return s;
    }
    // cout << inner << endl;
    string res;
    i = 0;
    while (inner.at(i) != 'E') {
        // cout << res << endl;
        ;
        int j = i;
        int count = 0;
        while (isdigit(inner.at(j))) {
            count = count * 10 + (inner.at(j) - '0');
            j++;
        }
        i = j;
        j = i + count;
        if (inner.at(i) == 'h' && inner.at(j) == 'E') {
            // cout << res << endl;
            bool is_all_hex = true;
            for (int k = i + 1; k < j; k++) {
                is_all_hex &= !!isxdigit(inner.at(k));
            }
            if (is_all_hex) {
                break;
            }
        }
        if (!res.empty()) {
            res.append("::");
        }
        if (inner.compare(i, 2, "_$") == 0) {
            i += 1;
        }
        while (true) {
            if (inner.at(i) == '.') {
                if (inner.at(i + 1) == '.') {
                    res.append("::");
                    i += 2;
                }
                else {
                    res.append(".");
                    i += 1;
                }
            }
            else if (inner.at(i) == '$') {
                i += 1;
                int next_dollar = inner.find("$", i);
                if (next_dollar == string::npos) {
                    break;
                }
                string escape = inner.substr(i, next_dollar - i);
                // string after_escape = inner.substr(next_dollar + 2);
                string unescape;
                if (escape.compare("SP") == 0) {
                    unescape = "@";
                }
                else if (escape.compare("BP") == 0) {
                    unescape = "*";
                }
                else if (escape.compare("RF") == 0) {
                    unescape = "&";
                }
                else if (escape.compare("LT") == 0) {
                    unescape = "<";
                }
                else if (escape.compare("GT") == 0) {
                    unescape = ">";
                }
                else if (escape.compare("LP") == 0) {
                    unescape = "(";
                }
                else if (escape.compare("RP") == 0) {
                    unescape = ")";
                }
                else if (escape.compare("C") == 0) {
                    unescape = ",";
                }
                else {
                    if (escape.at(0) == 'u') {
                        // int k = i + 1;
                        bool is_all_lower_hex = true;
                        int c = 0;
                        for (int k = 1; k < escape.size(); k++) {
                            is_all_lower_hex &= isxdigit(escape.at(k)) && !isupper(escape.at(k));
                            if (is_all_lower_hex) {
                                c = c * 16 + (escape.at(k) >= 'a' ? (escape.at(k) - 'a' + 10) : (escape.at(k) - '0'));
                            }
                        }
                        if (is_all_lower_hex && c < 128) {
                            if (!iscntrl(c)) {
                                // I don't know which control code of rust will flow to here;
                                res.push_back((char)c);
                                i = next_dollar + 1;
                                continue;
                            }
                        }
                    }
                    break;
                }
                res.append(unescape);
                i = next_dollar + 1;
            }
            else if (inner.find("$", i) != string::npos || inner.find(".", i) != string::npos) {
                int next = min<size_t>(inner.find("$", i), inner.find(".", i));
                if (next >= j) {
                    break;
                }
                res.append(inner.substr(i, next - i));
                i = next;
            }
            else {
                break;
            }
        }
        res.append(inner.substr(i, j - i));
        i = j;
    }
    return res;
}