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

package thrift

// Message type constants in the Thrift protocol.
type TMessageType int32

const (
	INVALID_TMESSAGE_TYPE TMessageType = 0
	CALL                  TMessageType = 1
	REPLY                 TMessageType = 2
	EXCEPTION             TMessageType = 3
	ONEWAY                TMessageType = 4
)
