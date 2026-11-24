# Plan

## Symbol names and emit

Parsing only records relations: `name`, `container`, `parent_enum`. It does not set `full_name` or `full_name_without_package`, and it does not emit symbols into the pool.

Imports are resolved first, as a dependency tree. The file being built is last. After that tree is parsed, one walk sets the two full names and registers every symbol. A parent is named before its children. Binding runs after that registration. The walk already required by binding is where emit happens.

`full_name_without_package` is the scope path plus `name`. `full_name` prefixes the file package.

An `enum class` value includes the enum name in its path. A plain `enum` value uses the enclosing scope and does not insert the enum name. The enum type itself keeps its own name either way.

## Next

Import dependency analysis: bind the import graph before naming and binding the file being built.
