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

package org.apache.thrift;

import org.apache.thrift.protocol.TProtocol;

/**
 * A TServiceClient is used to communicate with a TService implementation
 * across protocols and transports.
 */
public interface TServiceClient {
  /**
   * Get the TProtocol being used as the input (read) protocol.
   * @return
   */
  public TProtocol getInputProtocol();
  /**
   * Get the TProtocol being used as the output (write) protocol.
   * @return
   */
  public TProtocol getOutputProtocol();
}
