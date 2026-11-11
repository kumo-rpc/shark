#include <shark/serialize.h>
#include <tests/test-util.h>
#include <tests/test.sk.h>
#include <shark/base/main.h>
#include <unistd.h>

/* This is the entry point of a fuzz target to be used with libFuzzer
 * or another fuzz driver.
 * Such a fuzz driver is used by the autotools target shark-llvm-fuzzer-testcase
 * when the environment variable LIB_FUZZING_ENGINE is defined
 * for instance LIB_FUZZING_ENGINE=-fsanitize=fuzzer for libFuzzer
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  kj::ArrayPtr<const uint8_t> array(Data, Size);
  kj::ArrayInputStream ais(array);

  KJ_IF_MAYBE(e, kj::runCatchingExceptions([&]() {
    shark::InputStreamMessageReader reader(ais);
    shark::_::checkTestMessage(reader.getRoot<shark::_::TestAllTypes>());
    shark::_::checkDynamicTestMessage(reader.getRoot<shark::DynamicStruct>(shark::Schema::from<shark::_::TestAllTypes>()));
    kj::str(reader.getRoot<shark::_::TestAllTypes>());
  })) {
    KJ_LOG(ERROR, "threw");
  }
  return 0;
}
