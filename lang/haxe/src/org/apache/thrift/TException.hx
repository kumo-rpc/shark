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

class TException {

    @:isVar
    public var errorID(default,null) : Int;
    @:isVar
    public var errorMsg(default,null) : String;


    public function new(msg : String = "", id : Int = 0) {
        errorID = id;
        errorMsg = msg;
    }

	public function toString() : String {
		var clsname = Type.getClassName( Type.getClass(this));
		return '${clsname}: ${errorMsg} (code ${errorID})';
	}
	
}
 