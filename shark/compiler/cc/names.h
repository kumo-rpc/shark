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

#pragma once

#include <string>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {
    class Descriptor;
    class EnumDescriptor;
    class EnumValueDescriptor;
    class FieldDescriptor;
}
    namespace shark::compiler::cc {

        // Returns the unqualified C++ name.
        //
        // For example, if you had:
        //   package foo.bar;
        //   message Baz { message Moo {} }
        // Then the non-qualified version would be:
        //   Baz_Moo
        std::string ClassName(const Descriptor* descriptor);
        std::string ClassName(const EnumDescriptor* enum_descriptor);

        // Returns the fully qualified C++ name.
        //
        // For example, if you had:
        //   package foo.bar;
        //   message Baz { message Moo {} }
        // Then the qualified ClassName for Moo would be:
        //   ::foo::bar::Baz_Moo
        std::string QualifiedClassName(const Descriptor* d);
        std::string QualifiedClassName(const EnumDescriptor* d);
        std::string QualifiedExtensionName(const FieldDescriptor* d);

        // Get the (unqualified) name that should be used for this field in C++ code.
        // The name is coerced to lower-case to emulate proto1 behavior.  People
        // should be using lowercase-with-underscores style for proto field names
        // anyway, so normally this just returns field->name().
        std::string FieldName(const FieldDescriptor* field);

        // Requires that this field is in a oneof. Returns the (unqualified) case
        // constant for this field.
        std::string OneofCaseConstantName(const FieldDescriptor* field);
        // Returns the quafilied case constant for this field.
        std::string QualifiedOneofCaseConstantName(const FieldDescriptor* field);

        // Get the (unqualified) name that should be used for this enum value in C++
        // code.
        std::string EnumValueName(const EnumValueDescriptor* enum_value);

        // Strips ".shark" or ".protodevel" from the end of a filename.
        PROTOC_EXPORT std::string StripProto(const std::string& filename);

}  // namespace shark::compiler::cc

#include <shark/port_undef.inc>
