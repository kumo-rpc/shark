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
#include <vector>
#include <map>

#include <shark/schema/fwd.h>
#include <shark/schema/descriptor.h>
#include <shark/schema/field_descriptor.h>
#include <shark/schema/file_descriptor.h>
#include <shark/schema/service_descriptor.h>
#include <shark/schema/enum_descriptor.h>
#include <shark/stubs/mutex.h>

// Must be included last.
#include <shark/port_def.inc>

namespace shark {

    // Used to construct descriptors.
    //
    // Normally you won't want to build your own descriptors.  Message classes
    // constructed by the protocol compiler will provide them for you.  However,
    // if you are implementing Message on your own, or if you are writing a
    // program which can operate on totally arbitrary types and needs to load
    // them from some sort of database, you might need to.
    //
    // Since Descriptors are composed of a whole lot of cross-linked bits of
    // data that would be a pain to put together manually, the
    // DescriptorPool class is provided to make the process easier.  It can
    // take a FileDescriptorProto (defined in descriptor.shark), validate it,
    // and convert it to a set of nicely cross-linked Descriptors.
    //
    // DescriptorPool also helps with memory management.  Descriptors are
    // composed of many objects containing static data and pointers to each
    // other.  In all likelihood, when it comes time to delete this data,
    // you'll want to delete it all at once.  In fact, it is not uncommon to
    // have a whole pool of descriptors all cross-linked with each other which
    // you wish to delete all at once.  This class represents such a pool, and
    // handles the memory management for you.
    //
    // You can also search for descriptors within a DescriptorPool by name, and
    // extensions by number.
    class SHARK_EXPORT DescriptorPool {
    public:
        // Create a normal, empty DescriptorPool.
        DescriptorPool();

        // Constructs a DescriptorPool that, when it can't find something among the
        // descriptors already in the pool, looks for it in the given
        // DescriptorDatabase.
        // Notes:
        // - If a DescriptorPool is constructed this way, its BuildFile*() methods
        //   must not be called (they will assert-fail).  The only way to populate
        //   the pool with descriptors is to call the Find*By*() methods.
        // - The Find*By*() methods may block the calling thread if the
        //   DescriptorDatabase blocks.  This in turn means that parsing messages
        //   may block if they need to look up extensions.
        // - The Find*By*() methods will use mutexes for thread-safety, thus making
        //   them slower even when they don't have to fall back to the database.
        //   In fact, even the Find*By*() methods of descriptor objects owned by
        //   this pool will be slower, since they will have to obtain locks too.
        // - An ErrorCollector may optionally be given to collect validation errors
        //   in files loaded from the database.  If not given, errors will be printed
        //   to GOOGLE_LOG(ERROR).  Remember that files are built on-demand, so this
        //   ErrorCollector may be called from any thread that calls one of the
        //   Find*By*() methods.
        // - The DescriptorDatabase must not be mutated during the lifetime of
        //   the DescriptorPool. Even if the client takes care to avoid data races,
        //   changes to the content of the DescriptorDatabase may not be reflected
        //   in subsequent lookups in the DescriptorPool.
        class ErrorCollector;
        explicit DescriptorPool(DescriptorDatabase* fallback_database,
            ErrorCollector* error_collector = nullptr);

        ~DescriptorPool();

        // Get a pointer to the generated pool.  Generated protocol message classes
        // which are compiled into the binary will allocate their descriptors in
        // this pool.  Do not add your own descriptors to this pool.
        static const DescriptorPool* generated_pool();

        // Find a FileDescriptor in the pool by file name.  Returns nullptr if not
        // found.
        const FileDescriptor* FindFileByName(ConstStringParam name) const;

        // Find the FileDescriptor in the pool which defines the given symbol.
        // If any of the Find*ByName() methods below would succeed, then this is
        // equivalent to calling that method and calling the result's file() method.
        // Otherwise this returns nullptr.
        const FileDescriptor* FindFileContainingSymbol(
            ConstStringParam symbol_name) const;

        // Looking up descriptors ------------------------------------------
        // These find descriptors by fully-qualified name.  These will find both
        // top-level descriptors and nested descriptors.  They return nullptr if not
        // found.

        const Descriptor* FindMessageTypeByName(ConstStringParam name) const;
        const FieldDescriptor* FindFieldByName(ConstStringParam name) const;
        const FieldDescriptor* FindExtensionByName(ConstStringParam name) const;
        const OneofDescriptor* FindOneofByName(ConstStringParam name) const;
        const EnumDescriptor* FindEnumTypeByName(ConstStringParam name) const;
        const EnumValueDescriptor* FindEnumValueByName(ConstStringParam name) const;
        const ServiceDescriptor* FindServiceByName(ConstStringParam name) const;
        const MethodDescriptor* FindMethodByName(ConstStringParam name) const;

        // Finds an extension of the given type by number.  The extendee must be
        // a member of this DescriptorPool or one of its underlays.
        const FieldDescriptor* FindExtensionByNumber(const Descriptor* extendee,
            int number) const;

        // Finds an extension of the given type by its printable name.
        // See comments above PrintableNameForExtension() for the definition of
        // "printable name".  The extendee must be a member of this DescriptorPool
        // or one of its underlays.  Returns nullptr if there is no known message
        // extension with the given printable name.
        const FieldDescriptor* FindExtensionByPrintableName(
            const Descriptor* extendee, ConstStringParam printable_name) const;

        // Finds extensions of extendee. The extensions will be appended to
        // out in an undefined order. Only extensions defined directly in
        // this DescriptorPool or one of its underlays are guaranteed to be
        // found: extensions defined in the fallback database might not be found
        // depending on the database implementation.
        void FindAllExtensions(const Descriptor* extendee,
            std::vector<const FieldDescriptor*>* out) const;

        // Building descriptors --------------------------------------------

        // When converting a FileDescriptorProto to a FileDescriptor, various
        // errors might be detected in the input.  The caller may handle these
        // programmatically by implementing an ErrorCollector.
        class SHARK_EXPORT ErrorCollector {
        public:
            inline ErrorCollector() { }
            virtual ~ErrorCollector();

            // These constants specify what exact part of the construct is broken.
            // This is useful e.g. for mapping the error back to an exact location
            // in a .shark file.
            enum ErrorLocation {
                NAME, // the symbol name, or the package name for files
                NUMBER, // field or extension range number
                TYPE, // field type
                EXTENDEE, // field extendee
                DEFAULT_VALUE, // field default value
                INPUT_TYPE, // method input type
                OUTPUT_TYPE, // method output type
                OPTION_NAME, // name in assignment
                OPTION_VALUE, // value in option assignment
                IMPORT, // import error
                OTHER // some other problem
            };

            // Reports an error in the FileDescriptorProto. Use this function if the
            // problem occurred should interrupt building the FileDescriptorProto.
            virtual void AddError(
                const std::string& filename, // File name in which the error occurred.
                const std::string& element_name, // Full name of the erroneous element.
                const Message* descriptor, // Descriptor of the erroneous element.
                ErrorLocation location, // One of the location constants, above.
                const std::string& message // Human-readable error message.
                ) = 0;

            // Reports a warning in the FileDescriptorProto. Use this function if the
            // problem occurred should NOT interrupt building the FileDescriptorProto.
            virtual void AddWarning(
                const std::string& /*filename*/, // File name in which the error
                                                 // occurred.
                const std::string& /*element_name*/, // Full name of the erroneous
                                                     // element.
                const Message* /*descriptor*/, // Descriptor of the erroneous element.
                ErrorLocation /*location*/, // One of the location constants, above.
                const std::string& /*message*/ // Human-readable error message.
            ) { }

        private:
            GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(ErrorCollector);
        };

        // Convert the FileDescriptorProto to real descriptors and place them in
        // this DescriptorPool.  All dependencies of the file must already be in
        // the pool.  Returns the resulting FileDescriptor, or nullptr if there were
        // problems with the input (e.g. the message was invalid, or dependencies
        // were missing).  Details about the errors are written to GOOGLE_LOG(ERROR).
        const FileDescriptor* BuildFile(const FileDescriptorProto& proto);

        // Same as BuildFile() except errors are sent to the given ErrorCollector.
        const FileDescriptor* BuildFileCollectingErrors(
            const FileDescriptorProto& proto, ErrorCollector* error_collector);

        // By default, it is an error if a FileDescriptorProto contains references
        // to types or other files that are not found in the DescriptorPool (or its
        // backing DescriptorDatabase, if any).  If you call
        // AllowUnknownDependencies(), however, then unknown types and files
        // will be replaced by placeholder descriptors (which can be identified by
        // the is_placeholder() method).  This can allow you to
        // perform some useful operations with a .shark file even if you do not
        // have access to other .shark files on which it depends.  However, some
        // heuristics must be used to fill in the gaps in information, and these
        // can lead to descriptors which are inaccurate.  For example, the
        // DescriptorPool may be forced to guess whether an unknown type is a message
        // or an enum, as well as what package it resides in.  Furthermore,
        // placeholder types will not be discoverable via FindMessageTypeByName()
        // and similar methods, which could confuse some descriptor-based algorithms.
        // Generally, the results of this option should be handled with extreme care.
        void AllowUnknownDependencies() { allow_unknown_ = true; }

        // By default, weak imports are allowed to be missing, in which case we will
        // use a placeholder for the dependency and convert the field to be an Empty
        // message field. If you call EnforceWeakDependencies(true), however, the
        // DescriptorPool will report a import not found error.
        void EnforceWeakDependencies(bool enforce) { enforce_weak_ = enforce; }

        // Internal stuff --------------------------------------------------
        // These methods MUST NOT be called from outside the proto2 library.
        // These methods may contain hidden pitfalls and may be removed in a
        // future library version.

        // Create a DescriptorPool which is overlaid on top of some other pool.
        // If you search for a descriptor in the overlay and it is not found, the
        // underlay will be searched as a backup.  If the underlay has its own
        // underlay, that will be searched next, and so on.  This also means that
        // files built in the overlay will be cross-linked with the underlay's
        // descriptors if necessary.  The underlay remains property of the caller;
        // it must remain valid for the lifetime of the newly-constructed pool.
        //
        // Example:  Say you want to parse a .shark file at runtime in order to use
        // its type with a DynamicMessage.  Say this .shark file has dependencies,
        // but you know that all the dependencies will be things that are already
        // compiled into the binary.  For ease of use, you'd like to load the types
        // right out of generated_pool() rather than have to parse redundant copies
        // of all these .sharks and runtime.  But, you don't want to add the parsed
        // types directly into generated_pool(): this is not allowed, and would be
        // bad design anyway.  So, instead, you could use generated_pool() as an
        // underlay for a new DescriptorPool in which you add only the new file.
        //
        // WARNING:  Use of underlays can lead to many subtle gotchas.  Instead,
        //   try to formulate what you want to do in terms of DescriptorDatabases.
        explicit DescriptorPool(const DescriptorPool* underlay);

        // Called by generated classes at init time to add their descriptors to
        // generated_pool.  Do NOT call this in your own code!  filename must be a
        // permanent string (e.g. a string literal).
        static void InternalAddGeneratedFile(const void* encoded_file_descriptor,
            int size);

        // Disallow [enforce_utf8 = false] in .shark files.
        void DisallowEnforceUtf8() { disallow_enforce_utf8_ = true; }

        // For internal use only:  Gets a non-const pointer to the generated pool.
        // This is called at static-initialization time only, so thread-safety is
        // not a concern.  If both an underlay and a fallback database are present,
        // the underlay takes precedence.
        static DescriptorPool* internal_generated_pool();

        // For internal use only:  Gets a non-const pointer to the generated
        // descriptor database.
        // Only used for testing.
        static DescriptorDatabase* internal_generated_database();

        // For internal use only:  Changes the behavior of BuildFile() such that it
        // allows the file to make reference to message types declared in other files
        // which it did not officially declare as dependencies.
        void InternalDontEnforceDependencies();

        // For internal use only: Enables lazy building of dependencies of a file.
        // Delay the building of dependencies of a file descriptor until absolutely
        // necessary, like when message_type() is called on a field that is defined
        // in that dependency's file. This will cause functional issues if a proto
        // or one of its dependencies has errors. Should only be enabled for the
        // generated_pool_ (because no descriptor build errors are guaranteed by
        // the compilation generation process), testing, or if a lack of descriptor
        // build errors can be guaranteed for a pool.
        void InternalSetLazilyBuildDependencies() {
            lazily_build_dependencies_ = true;
            // This needs to be set when lazily building dependencies, as it breaks
            // dependency checking.
            InternalDontEnforceDependencies();
        }

        // For internal use only.
        void internal_set_underlay(const DescriptorPool* underlay) {
            underlay_ = underlay;
        }

        // For internal (unit test) use only:  Returns true if a FileDescriptor has
        // been constructed for the given file, false otherwise.  Useful for testing
        // lazy descriptor initialization behavior.
        bool InternalIsFileLoaded(ConstStringParam filename) const;

        // Add a file to unused_import_track_files_. DescriptorBuilder will log
        // warnings or errors for those files if there is any unused import.
        void AddUnusedImportTrackFile(ConstStringParam file_name,
            bool is_error = false);
        void ClearUnusedImportTrackFiles();

    private:
        friend class Descriptor;
        friend class internal::LazyDescriptor;
        friend class FieldDescriptor;
        friend class EnumDescriptor;
        friend class ServiceDescriptor;
        friend class MethodDescriptor;
        friend class FileDescriptor;
        friend class DescriptorBuilder;
        friend class FileDescriptorTables;

        // Return true if the given name is a sub-symbol of any non-package
        // descriptor that already exists in the descriptor pool.  (The full
        // definition of such types is already known.)
        bool IsSubSymbolOfBuiltType(StringPiece name) const;

        // Tries to find something in the fallback database and link in the
        // corresponding proto file.  Returns true if successful, in which case
        // the caller should search for the thing again.  These are declared
        // const because they are called by (semantically) const methods.
        bool TryFindFileInFallbackDatabase(StringPiece name) const;
        bool TryFindSymbolInFallbackDatabase(StringPiece name) const;
        bool TryFindExtensionInFallbackDatabase(const Descriptor* containing_type,
            int field_number) const;

        // This internal find extension method only check with its table and underlay
        // descriptor_pool's table. It does not check with fallback DB and no
        // additional proto file will be build in this method.
        const FieldDescriptor* InternalFindExtensionByNumberNoLock(
            const Descriptor* extendee, int number) const;

        // Like BuildFile() but called internally when the file has been loaded from
        // fallback_database_.  Declared const because it is called by (semantically)
        // const methods.
        const FileDescriptor* BuildFileFromDatabase(
            const FileDescriptorProto& proto) const;

        // Helper for when lazily_build_dependencies_ is set, can look up a symbol
        // after the file's descriptor is built, and can build the file where that
        // symbol is defined if necessary. Will create a placeholder if the type
        // doesn't exist in the fallback database, or the file doesn't build
        // successfully.
        Symbol CrossLinkOnDemandHelper(StringPiece name,
            bool expecting_enum) const;

        // Create a placeholder FileDescriptor of the specified name
        FileDescriptor* NewPlaceholderFile(StringPiece name) const;
        FileDescriptor* NewPlaceholderFileWithMutexHeld(
            StringPiece name, internal::FlatAllocator& alloc) const;

        enum PlaceholderType {
            PLACEHOLDER_MESSAGE,
            PLACEHOLDER_ENUM,
            PLACEHOLDER_EXTENDABLE_MESSAGE
        };
        // Create a placeholder Descriptor of the specified name
        Symbol NewPlaceholder(StringPiece name,
            PlaceholderType placeholder_type) const;
        Symbol NewPlaceholderWithMutexHeld(StringPiece name,
            PlaceholderType placeholder_type) const;

        // If fallback_database_ is nullptr, this is nullptr.  Otherwise, this is a
        // mutex which must be locked while accessing tables_.
        internal::WrappedMutex* mutex_;

        // See constructor.
        DescriptorDatabase* fallback_database_;
        ErrorCollector* default_error_collector_;
        const DescriptorPool* underlay_;

        // This class contains a lot of hash maps with complicated types that
        // we'd like to keep out of the header.
        class Tables;
        std::unique_ptr<Tables> tables_;

        bool enforce_dependencies_;
        bool lazily_build_dependencies_;
        bool allow_unknown_;
        bool enforce_weak_;
        bool disallow_enforce_utf8_;

        // Set of files to track for unused imports. The bool value when true means
        // unused imports are treated as errors (and as warnings when false).
        std::map<std::string, bool> unused_import_track_files_;

        GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(DescriptorPool);
    };

}  // namespace shark

#include <shark/port_undef.inc>
