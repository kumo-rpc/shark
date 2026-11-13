#ifndef SHARK_TESTS_GTEST_ENUM_H_
#define SHARK_TESTS_GTEST_ENUM_H_

#include <type_traits>

// gtest compares with operator==. enum class does not compare with int, and
// a CmpHelperEQ overload declared after gtest is not visible to EqHelper.
// Operators in the enum's namespace are found by argument-dependent lookup.
#define SHARK_TEST_ENUM_INT_COMPARE(ns)                                     \
  namespace ns {                                                            \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator==(T lhs, U rhs) {                                           \
    return static_cast<U>(lhs) == rhs;                                      \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator==(U lhs, T rhs) {                                           \
    return lhs == static_cast<U>(rhs);                                      \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator!=(T lhs, U rhs) {                                           \
    return static_cast<U>(lhs) != rhs;                                      \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator!=(U lhs, T rhs) {                                           \
    return lhs != static_cast<U>(rhs);                                      \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator<(T lhs, U rhs) {                                            \
    return static_cast<U>(lhs) < rhs;                                       \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator<(U lhs, T rhs) {                                            \
    return lhs < static_cast<U>(rhs);                                       \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator>(T lhs, U rhs) {                                            \
    return static_cast<U>(lhs) > rhs;                                       \
  }                                                                         \
  template <typename T, typename U,                                         \
            typename std::enable_if<std::is_enum<T>::value &&               \
                                        std::is_integral<U>::value,         \
                                    int>::type = 0>                         \
  bool operator>(U lhs, T rhs) {                                            \
    return lhs > static_cast<U>(rhs);                                       \
  }                                                                         \
  }

SHARK_TEST_ENUM_INT_COMPARE(sharkbuf_unittest)
SHARK_TEST_ENUM_INT_COMPARE(sharkbuf_unittest::no_generic_services_test)
SHARK_TEST_ENUM_INT_COMPARE(sharkbuf_unittest_import)
SHARK_TEST_ENUM_INT_COMPARE(protobuf_test_messages::proto2)
SHARK_TEST_ENUM_INT_COMPARE(protobuf_test_messages::proto3)
SHARK_TEST_ENUM_INT_COMPARE(proto3_unittest)
SHARK_TEST_ENUM_INT_COMPARE(proto3)

#undef SHARK_TEST_ENUM_INT_COMPARE

#endif  // SHARK_TESTS_GTEST_ENUM_H_
