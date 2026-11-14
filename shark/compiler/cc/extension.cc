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

#include <shark/compiler/cc/extension.h>

#include <map>

#include <shark/compiler/cc/helpers.h>
#include <shark/descriptor.skp.h>
#include <shark/io/printer.h>
#include <shark/stubs/strutil.h>

namespace shark::compiler::cc {

    ExtensionGenerator::ExtensionGenerator(const FieldDescriptor* descriptor,
        const Options& options,
        MessageSCCAnalyzer* scc_analyzer)
        : descriptor_(descriptor)
        , options_(options)
        , scc_analyzer_(scc_analyzer) {
        // Construct type_traits_.
        if (descriptor_->is_repeated()) {
            type_traits_ = "Repeated";
        }

        switch (descriptor_->cpp_type()) {
        case SharkFieldCppType::CPPTYPE_ENUM:
            type_traits_.append("EnumTypeTraits< ");
            type_traits_.append(ClassName(descriptor_->enum_type(), true));
            type_traits_.append(", ");
            type_traits_.append(ClassName(descriptor_->enum_type(), true));
            type_traits_.append(IsEnumClass(descriptor_->enum_type())
                    ? "Trait::fits_underlying>"
                    : "Trait::is_valid>");
            break;
        case SharkFieldCppType::CPPTYPE_STRING:
            type_traits_.append("StringTypeTraits");
            break;
        case SharkFieldCppType::CPPTYPE_MESSAGE:
            type_traits_.append("MessageTypeTraits< ");
            type_traits_.append(ClassName(descriptor_->message_type(), true));
            type_traits_.append(" >");
            break;
        default:
            type_traits_.append("PrimitiveTypeTraits< ");
            type_traits_.append(PrimitiveTypeName(options_, descriptor_->cpp_type()));
            type_traits_.append(" >");
            break;
        }
        SetCommonVars(options, &variables_);
        SetCommonMessageDataVariables(descriptor_->containing_type(), &variables_);
        variables_["extendee"] = QualifiedClassName(descriptor_->containing_type(), options_);
        variables_["type_traits"] = type_traits_;
        std::string name = descriptor_->name();
        variables_["name"] = ResolveKeyword(name);
        variables_["constant_name"] = FieldConstantName(descriptor_);
        variables_["field_type"] = StrCat(static_cast<int>(descriptor_->type()));
        variables_["packed"] = descriptor_->is_packed() ? "true" : "false";

        std::string scope = IsScoped() ? ClassName(descriptor_->extension_scope(), false) + "::" : "";
        variables_["scope"] = scope;
        variables_["scoped_name"] = ExtensionName(descriptor_);
        variables_["number"] = StrCat(descriptor_->number());

        bool add_verify_fn =
            // Only verify msgs.
            descriptor_->cpp_type() == SharkFieldCppType::CPPTYPE_MESSAGE &&
            // Options say to verify.
            ShouldVerify(descriptor_->message_type(), options_, scc_analyzer_) && ShouldVerify(descriptor_->containing_type(), options_, scc_analyzer_);

        variables_["verify_fn"] = add_verify_fn
            ? StrCat("&", FieldMessageTypeName(descriptor_, options_),
                  "::InternalVerify")
            : "nullptr";
    }

    ExtensionGenerator::~ExtensionGenerator() { }

    bool ExtensionGenerator::IsScoped() const {
        return descriptor_->extension_scope() != nullptr;
    }

    void ExtensionGenerator::GenerateDeclaration(io::Printer* printer) const {
        Formatter format(printer, variables_);

        // If this is a class member, it needs to be declared "static".  Otherwise,
        // it needs to be "extern".  In the latter case, it also needs the DLL
        // export/import specifier.
        std::string qualifier;
        if (!IsScoped()) {
            qualifier = "extern";
            if (!options_.dllexport_decl.empty()) {
                qualifier = options_.dllexport_decl + " " + qualifier;
            }
        } else {
            qualifier = "static";
        }

        format(
            "static const int $constant_name$ = $number$;\n"
            "$1$ ::$proto_ns$::internal::ExtensionIdentifier< $extendee$,\n"
            "    ::$proto_ns$::internal::$type_traits$, $field_type$, $packed$ >\n"
            "  ${2$$name$$}$;\n",
            qualifier, descriptor_);
    }

    void ExtensionGenerator::GenerateDefinition(io::Printer* printer) {
        // If we are building for lite with implicit weak fields, we want to skip over
        // any custom options (i.e. extensions of messages from descriptor.shark).
        // This prevents the creation of any unnecessary linker references to the
        // descriptor messages.
        if (options_.lite_implicit_weak_fields && descriptor_->containing_type()->file()->name() == "net/proto2/proto/descriptor.shark") {
            return;
        }

        Formatter format(printer, variables_);
        std::string default_str;
        // If this is a class member, it needs to be declared in its class scope.
        if (descriptor_->cpp_type() == SharkFieldCppType::CPPTYPE_STRING) {
            // We need to declare a global string which will contain the default value.
            // We cannot declare it at class scope because that would require exposing
            // it in the header which would be annoying for other reasons.  So we
            // replace :: with _ in the name and declare it as a global.
            default_str = StringReplace(variables_["scoped_name"], "::", "_", true) + "_default";
            format("const std::string $1$($2$);\n", default_str,
                DefaultValue(options_, descriptor_));
        } else if (descriptor_->message_type()) {
            // We have to initialize the default instance for extensions at registration
            // time.
            default_str = FieldMessageTypeName(descriptor_, options_) + "::default_instance()";
        } else {
            default_str = DefaultValue(options_, descriptor_);
        }

        // Likewise, class members need to declare the field constant variable.
        if (IsScoped()) {
            format(
                "#if !defined(_MSC_VER) || (_MSC_VER >= 1900 && _MSC_VER < 1912)\n"
                "const int $scope$$constant_name$;\n"
                "#endif\n");
        }

        format(
            "SHARK_ATTRIBUTE_INIT_PRIORITY2 "
            "::$proto_ns$::internal::ExtensionIdentifier< $extendee$,\n"
            "    ::$proto_ns$::internal::$type_traits$, $field_type$, $packed$>\n"
            "  $scoped_name$($constant_name$, $1$, $verify_fn$);\n",
            default_str);
    }

} // namespace shark::compiler::cc
