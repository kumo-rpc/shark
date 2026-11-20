// Copyright (C) 2026 Kumo inc. and its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#define PY_SSIZE_T_CLEAN
#include "ext/binary.h"
namespace apache {
namespace thrift {
namespace py {

bool BinaryProtocol::readFieldBegin(TType& type, int16_t& tag) {
  uint8_t b = 0;
  if (!readByte(b)) {
    return false;
  }
  type = static_cast<TType>(b);
  if (type == T_STOP) {
    return true;
  }
  return readI16(tag);
}
}
}
}
