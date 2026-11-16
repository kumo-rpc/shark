// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// from google3/strings/strutil.cc

#include <shark/stubs/strutil.h>
#include <turbo/strings/numbers.h>

#include <cmath>
#include <errno.h>
#include <float.h> // FLT_DIG and DBL_DIG
#include <iterator>
#include <limits.h>
#include <limits>
#include <stdio.h>

#include <shark/stubs/logging.h>
#include <shark/stubs/stl_util.h>

#ifdef _WIN32
// MSVC has only _snprintf, not snprintf.
//
// MinGW has both snprintf and _snprintf, but they appear to be different
// functions.  The former is buggy.  When invoked like so:
//   char buffer[32];
//   snprintf(buffer, 32, "%.*g\n", FLT_DIG, 1.23e10f);
// it prints "1.23000e+10".  This is plainly wrong:  %g should never print
// trailing zeros after the decimal point.  For some reason this bug only
// occurs with some input values, not all.  In any case, _snprintf does the
// right thing, so we use it.
#define snprintf _snprintf
#endif

namespace shark {

    namespace internal {

        // ----------------------------------------------------------------------
        // NoLocaleStrtod()
        //   This code will make you cry.
        // ----------------------------------------------------------------------

        namespace {

            // Returns a string identical to *input except that the character pointed to
            // by radix_pos (which should be '.') is replaced with the locale-specific
            // radix character.
            std::string LocalizeRadix(const char* input, const char* radix_pos) {
                // Determine the locale-specific radix character by calling sprintf() to
                // print the number 1.5, then stripping off the digits.  As far as I can
                // tell, this is the only portable, thread-safe way to get the C library
                // to divuldge the locale's radix character.  No, localeconv() is NOT
                // thread-safe.
                char temp[16];
                int size = snprintf(temp, sizeof(temp), "%.1f", 1.5);
                GOOGLE_CHECK_EQ(temp[0], '1');
                GOOGLE_CHECK_EQ(temp[size - 1], '5');
                GOOGLE_CHECK_LE(size, 6);

                // Now replace the '.' in the input with it.
                std::string result;
                result.reserve(strlen(input) + size - 3);
                result.append(input, radix_pos);
                result.append(temp + 1, size - 2);
                result.append(radix_pos + 1);
                return result;
            }

        } // namespace

        double NoLocaleStrtod(const char* str, char** endptr) {
            // We cannot simply set the locale to "C" temporarily with setlocale()
            // as this is not thread-safe.  Instead, we try to parse in the current
            // locale first.  If parsing stops at a '.' character, then this is a
            // pretty good hint that we're actually in some other locale in which
            // '.' is not the radix character.

            char* temp_endptr;
            double result = strtod(str, &temp_endptr);
            if (endptr != NULL)
                *endptr = temp_endptr;
            if (*temp_endptr != '.')
                return result;

            // Parsing halted on a '.'.  Perhaps we're in a different locale?  Let's
            // try to replace the '.' with a locale-specific radix character and
            // try again.
            std::string localized = LocalizeRadix(str, temp_endptr);
            const char* localized_cstr = localized.c_str();
            char* localized_endptr;
            result = strtod(localized_cstr, &localized_endptr);
            if ((localized_endptr - localized_cstr) > (temp_endptr - str)) {
                // This attempt got further, so replacing the decimal must have helped.
                // Update endptr to point at the right location.
                if (endptr != NULL) {
                    // size_diff is non-zero if the localized radix has multiple bytes.
                    int size_diff = localized.size() - strlen(str);
                    // const_cast is necessary to match the strtod() interface.
                    *endptr = const_cast<char*>(
                        str + (localized_endptr - localized_cstr - size_diff));
                }
            }

            return result;
        }

    } // namespace internal

} // namespace shark
