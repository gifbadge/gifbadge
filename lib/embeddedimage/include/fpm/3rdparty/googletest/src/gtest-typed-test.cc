/*******************************************************************************
 * Copyright (c) 2026 GifBadge
 *
 * SPDX-License-Identifier:   GPL-3.0-or-later
 ******************************************************************************/


#include "gtest/gtest-typed-test.h"

#include "gtest/gtest.h"

namespace testing {
namespace internal {

#if GTEST_HAS_TYPED_TEST_P

// Skips to the first non-space char in str. Returns an empty string if str
// contains only whitespace characters.
static const char* SkipSpaces(const char* str) {
  while (IsSpace(*str))
    str++;
  return str;
}

static std::vector<std::string> SplitIntoTestNames(const char* src) {
  std::vector<std::string> name_vec;
  src = SkipSpaces(src);
  for (; src != nullptr; src = SkipComma(src)) {
    name_vec.push_back(StripTrailingSpaces(GetPrefixUntilComma(src)));
  }
  return name_vec;
}

// Verifies that registered_tests match the test names in
// registered_tests_; returns registered_tests if successful, or
// aborts the program otherwise.
const char* TypedTestSuitePState::VerifyRegisteredTestNames(
    const char* file, int line, const char* registered_tests) {
  typedef RegisteredTestsMap::const_iterator RegisteredTestIter;
  registered_ = true;

  std::vector<std::string> name_vec = SplitIntoTestNames(registered_tests);

  Message errors;

  std::set<std::string> tests;
  for (std::vector<std::string>::const_iterator name_it = name_vec.begin();
       name_it != name_vec.end(); ++name_it) {
    const std::string& name = *name_it;
    if (tests.count(name) != 0) {
      errors << "Test " << name << " is listed more than once.\n";
      continue;
    }

    bool found = false;
    for (RegisteredTestIter it = registered_tests_.begin();
         it != registered_tests_.end();
         ++it) {
      if (name == it->first) {
        found = true;
        break;
      }
    }

    if (found) {
      tests.insert(name);
    } else {
      errors << "No test named " << name
             << " can be found in this test suite.\n";
    }
  }

  for (RegisteredTestIter it = registered_tests_.begin();
       it != registered_tests_.end();
       ++it) {
    if (tests.count(it->first) == 0) {
      errors << "You forgot to list test " << it->first << ".\n";
    }
  }

  const std::string& errors_str = errors.GetString();
  if (errors_str != "") {
    fprintf(stderr, "%s %s", FormatFileLocation(file, line).c_str(),
            errors_str.c_str());
    fflush(stderr);
    posix::Abort();
  }

  return registered_tests;
}

#endif  // GTEST_HAS_TYPED_TEST_P

}  // namespace internal
}  // namespace testing
