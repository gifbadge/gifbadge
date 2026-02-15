/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/

// The Google C++ Testing and Mocking Framework (Google Test)
//
// This file implements just enough of the matcher interface to allow
// EXPECT_DEATH and friends to accept a matcher argument.

#include "gtest/internal/gtest-internal.h"
#include "gtest/internal/gtest-port.h"
#include "gtest/gtest-matchers.h"

#include <string>

namespace testing {

// Constructs a matcher that matches a const std::string& whose value is
// equal to s.
Matcher<const std::string&>::Matcher(const std::string& s) { *this = Eq(s); }

#if GTEST_HAS_GLOBAL_STRING
// Constructs a matcher that matches a const std::string& whose value is
// equal to s.
Matcher<const std::string&>::Matcher(const ::string& s) {
  *this = Eq(static_cast<std::string>(s));
}
#endif  // GTEST_HAS_GLOBAL_STRING

// Constructs a matcher that matches a const std::string& whose value is
// equal to s.
Matcher<const std::string&>::Matcher(const char* s) {
  *this = Eq(std::string(s));
}

// Constructs a matcher that matches a std::string whose value is equal to
// s.
Matcher<std::string>::Matcher(const std::string& s) { *this = Eq(s); }

#if GTEST_HAS_GLOBAL_STRING
// Constructs a matcher that matches a std::string whose value is equal to
// s.
Matcher<std::string>::Matcher(const ::string& s) {
  *this = Eq(static_cast<std::string>(s));
}
#endif  // GTEST_HAS_GLOBAL_STRING

// Constructs a matcher that matches a std::string whose value is equal to
// s.
Matcher<std::string>::Matcher(const char* s) { *this = Eq(std::string(s)); }

#if GTEST_HAS_GLOBAL_STRING
// Constructs a matcher that matches a const ::string& whose value is
// equal to s.
Matcher<const ::string&>::Matcher(const std::string& s) {
  *this = Eq(static_cast<::string>(s));
}

// Constructs a matcher that matches a const ::string& whose value is
// equal to s.
Matcher<const ::string&>::Matcher(const ::string& s) { *this = Eq(s); }

// Constructs a matcher that matches a const ::string& whose value is
// equal to s.
Matcher<const ::string&>::Matcher(const char* s) { *this = Eq(::string(s)); }

// Constructs a matcher that matches a ::string whose value is equal to s.
Matcher<::string>::Matcher(const std::string& s) {
  *this = Eq(static_cast<::string>(s));
}

// Constructs a matcher that matches a ::string whose value is equal to s.
Matcher<::string>::Matcher(const ::string& s) { *this = Eq(s); }

// Constructs a matcher that matches a string whose value is equal to s.
Matcher<::string>::Matcher(const char* s) { *this = Eq(::string(s)); }
#endif  // GTEST_HAS_GLOBAL_STRING

#if GTEST_HAS_ABSL
// Constructs a matcher that matches a const absl::string_view& whose value is
// equal to s.
Matcher<const absl::string_view&>::Matcher(const std::string& s) {
  *this = Eq(s);
}

#if GTEST_HAS_GLOBAL_STRING
// Constructs a matcher that matches a const absl::string_view& whose value is
// equal to s.
Matcher<const absl::string_view&>::Matcher(const ::string& s) { *this = Eq(s); }
#endif  // GTEST_HAS_GLOBAL_STRING

// Constructs a matcher that matches a const absl::string_view& whose value is
// equal to s.
Matcher<const absl::string_view&>::Matcher(const char* s) {
  *this = Eq(std::string(s));
}

// Constructs a matcher that matches a const absl::string_view& whose value is
// equal to s.
Matcher<const absl::string_view&>::Matcher(absl::string_view s) {
  *this = Eq(std::string(s));
}

// Constructs a matcher that matches a absl::string_view whose value is equal to
// s.
Matcher<absl::string_view>::Matcher(const std::string& s) { *this = Eq(s); }

#if GTEST_HAS_GLOBAL_STRING
// Constructs a matcher that matches a absl::string_view whose value is equal to
// s.
Matcher<absl::string_view>::Matcher(const ::string& s) { *this = Eq(s); }
#endif  // GTEST_HAS_GLOBAL_STRING

// Constructs a matcher that matches a absl::string_view whose value is equal to
// s.
Matcher<absl::string_view>::Matcher(const char* s) {
  *this = Eq(std::string(s));
}

// Constructs a matcher that matches a absl::string_view whose value is equal to
// s.
Matcher<absl::string_view>::Matcher(absl::string_view s) {
  *this = Eq(std::string(s));
}
#endif  // GTEST_HAS_ABSL

}  // namespace testing
