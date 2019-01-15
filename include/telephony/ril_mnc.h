/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RIL_MNC_H
#define RIL_MNC_H

#include <climits>
#include <cstdio>
#include <string>

namespace ril {
namespace util {
namespace mnc {

/**
 * Decode an MNC with an optional length indicator provided in the most-significant nibble.
 *
 * @param mnc an encoded MNC value; if no encoding is provided, then the string is returned
 *     as a minimum length string representing the provided integer.
 *
 * @return string representation of an encoded MNC or an empty string if the MNC is not a valid
 *     MNC value.
 */
static inline std::string decode(int mnc) {
    if (mnc == INT_MAX || mnc < 0) return "";
    unsigned umnc = mnc;
    char mncNumDigits = (umnc >> (sizeof(int) * 8 - 4)) & 0xF;

    umnc = (umnc << 4) >> 4;
    if (umnc > 999) return "";

    char mncStr[4] = {0};
    switch (mncNumDigits) {
        case 0:
            // Legacy MNC report hasn't set the number of digits; preserve current
            // behavior and make a string of the minimum number of required digits.
            return std::to_string(umnc);

        case 2:
            snprintf(mncStr, sizeof(mncStr), "%03.3u", umnc);
            return mncStr + 1;

        case 3:
            snprintf(mncStr, sizeof(mncStr), "%03.3u", umnc);
            return mncStr;

        default:
            // Error case
            return "";
    }

}

/**
 * Encode an MNC of the given value and a given number of digits
 *
 * @param mnc an MNC value 0-999 or INT_MAX if unknown
 * @param numDigits the number of MNC digits {2, 3} or 0 if unknown
 *
 * @return an encoded MNC with embedded length information
 */
static inline int encode(int mnc, int numDigits) {
    if (mnc > 999 || mnc < 0) return INT_MAX;
    switch (numDigits) {
        case 0: // fall through
        case 2: // fall through
        case 3:
            break;

        default:
            return INT_MAX;
    };

    return (numDigits << (sizeof(int) * 8 - 4)) | mnc;
}

/**
 * Encode an MNC of the given value
 *
 * @param mnc the string representation of the MNC, with the length equal to the length of the
 *     provided string.
 *
 * @return an encoded MNC with embedded length information
 */
static inline int encode(const std::string & mnc) {
    return encode(std::stoi(mnc), mnc.length());
}

// echo -e "#include \"hardware/ril/include/telephony/ril_mnc.h\"\nint main()"\
// "{ return ril::util::mnc::test(); }" > ril_test.cpp \
// && g++ -o /tmp/ril_test -DTEST_RIL_MNC ril_test.cpp; \
// rm ril_test.cpp; /tmp/ril_test && [ $? ] && echo "passed"
#ifdef TEST_RIL_MNC
static int test() {
    const struct mnc_strings { const char * in; const char * out; } mncs[] = {
        {"0001",""},
        {"9999",""},
        {"0",""},
        {"9",""},
        {"123","123"},
        {"000","000"},
        {"001","001"},
        {"011","011"},
        {"111","111"},
        {"00","00"},
        {"01","01"},
        {"11","11"},
        {"09","09"},
        {"099","099"},
        {"999", "999"}};

    for (int i=0; i< sizeof(mncs) / sizeof(struct mnc_strings); i++) {
        if (decode(encode(mncs[i].in)).compare(mncs[i].out)) return 1;
    }

    const struct mnc_ints { const int in; const char * out; } legacy_mncs[] = {
        {INT_MAX, ""},
        {1, "1"},
        {11, "11"},
        {111, "111"},
        {0, "0"},
        {9999, ""},
    };

    for (int i=0; i < sizeof(legacy_mncs) / sizeof(struct mnc_ints); i++) {
        if (decode(legacy_mncs[i].in).compare(legacy_mncs[i].out)) return 1;
    }

    return 0;
}
#endif

}
}
}
#endif /* !defined(RIL_MNC_H) */
