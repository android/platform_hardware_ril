/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef RIL_MCC_H
#define RIL_MCC_H

#include <climits>
#include <cstdio>
#include <string>

namespace ril {
namespace util {
namespace mcc {

/**
 * Decode an integer mcc and encode as 3 digit string
 *
 * @param an integer mcc, its range should be in 0 to 999.
 *
 * @return string representation of an encoded MCC or an empty string
 * if the MCC is not a valid MCC value.
 */
static inline std::string decode(int mcc) {
    char mccStr[4] = {0};
    if (mcc > 999 || mcc < 0) return "";

    snprintf(mccStr, sizeof(mccStr), "%03d", mcc);
    return mccStr;
}

// echo -e "#include \"hardware/ril/include/telephony/ril_mcc.h\"\nint main()"\
// "{ return ril::util::mcc::test(); }" > ril_test.cpp \
// && g++ -o /tmp/ril_test -DTEST_RIL_MCC ril_test.cpp; \
// rm ril_test.cpp; /tmp/ril_test && [ $? ] && echo "passed"
#ifdef TEST_RIL_MCC
static int test() {
    const struct mcc_ints { const int in; const char * out; } legacy_mccs[] = {
        {INT_MAX, ""},
        {1, "001"},
        {11, "011"},
        {111, "111"},
        {0, "000"},
        {9999, ""},
        {-12, ""},
    };

    for (int i=0; i < sizeof(legacy_mccs) / sizeof(struct mcc_ints); i++) {
        if (decode(legacy_mccs[i].in).compare(legacy_mccs[i].out)) return 1;
    }

    return 0;
}
#endif

}
}
}
#endif /* !defined(RIL_MCC_H) */
