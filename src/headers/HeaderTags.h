//
// Created by Hector van der Aa on 6/13/25.
//

#ifndef HEADERTAGS_H
#define HEADERTAGS_H

#include <iostream>

namespace sk::headers {
    struct Tag {char v[4];};
    inline std::ostream& operator<<(std::ostream& os, const Tag& input) {
        for (char c : input.v) {
            os << c;
        }
        return os;
    }

    struct PascalString {
        uint8_t Size;
        char v[13];
    };
}

#endif //HEADERTAGS_H
