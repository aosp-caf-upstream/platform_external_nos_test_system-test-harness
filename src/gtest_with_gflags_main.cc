
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <iostream>
#include <sstream>

#ifdef ANDROID
#define FLAGS_list_slow_tests false
#define FLAGS_disable_slow_tests false
#else
#include <gflags/gflags.h>

DEFINE_bool(list_slow_tests, false, "List tests included in the set of slow tests.");
DEFINE_bool(disable_slow_tests, false, "Enables a filter to disable a set of slow tests.");
#endif  // ANDROID

int main(int argc, char** argv) {
  const std::vector<std::string> slow_tests{
      "AvbTest.*",
      "ImportKeyTest.RSASuccess",
      "NuggetCoreTest.HardRebootTest",
      "WeaverTest.WriteHardRebootRead",
      "WeaverTest.ReadThrottleAfterHardReboot",
      "WeaverTest.ReadThrottleAfterSleep",
      "WeaverTest.ReadAttemptCounterPersistsHardReboot",
  };

  testing::InitGoogleMock(&argc, argv);
#ifndef ANDROID
  google::ParseCommandLineFlags(&argc, &argv, true);
#endif  // ANDROID

  if (FLAGS_list_slow_tests) {
    std::cout << "Slow tests:\n";
    for (const auto& test : slow_tests) {
      std::cout << "  " << test << "\n";
    }
    std::cout.flush();
    exit(0);
  }

  if (FLAGS_disable_slow_tests) {
    std::stringstream ss;
    bool first = true;
    for (const auto& test : slow_tests) {
      if (first) {
        first = false;
        ss << "-";
      } else {
        ss << ":";
      }
      ss << test;
    }
    ::testing::GTEST_FLAG(filter) = ss.str();
  }

  return RUN_ALL_TESTS();
}
