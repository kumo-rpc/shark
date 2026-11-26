# Schema

`shark/schema/schema_data.h` 是可序列化的文件模型。生成静态代码、导出 JSON、再装回 `RegistryPool` 都以它为准。

`shark/parser/file.h` 的 `FileNode` 是运行时 AST。成员是节点指针、`Lexer` 和 `RegistryPool`，不能直接序列化。

这一阶段只定结构，没有做 AST 到 schema 的转换，也没有做生成器。

## 值模型

- 成员是 `string`、`vector`、枚举这些值。schema 里不放 `StructNode*`、`FileNode*` 这种 parser 节点指针。
- `std::optional` 只用在 `FileData::edition`。文件里没有 edition 声明时，这个字段就是空的。
- 每个声明带 `SourceSite`：1-based 的行和列，以及同一层从 0 开始的 `sequence`。
- 序号记在元素自己身上。文件顶层共用一层；struct 内部的 field、嵌套类型、alias 另起一层。
- 不另做 `DeclRef` / `order` 下标表。
- 不保存绝对路径，不保存 include 搜索路径。`ImportInfo::path` 是源码里写的 import。
- `FileData::package` 已删除，package 声明只在 `packages`。
- `indirect_dependencies` 只留给调试。生成代码和装回 pool 都不用它。
- 源文件正文不进 schema。
- `Service` 只有前向声明，没有对应结构。`InterfaceNode` 不在 `FileNode` 上，也不做。
- 注释结构 `CommentInfo` 已有，但还没有挂到各个声明上。文件级注释在 `FileData::comments`。

装回 pool 时，用包名、嵌套路径和 `name` 拼出 `full_name`，再 `add_symbol` / `add_file`。节点里的 `string_view` 必须指向仍活着的 `FileData` 字符串。类型、别名目标、servlet 基类这些指针 schema 里没有，装入后再按名字绑定。

## 已有结构

`EditionInfo`、`CommentInfo`、`PackageInfo`、`ImportInfo`、`Value`、`AnnotationInfo`、`EnumValueInfo`、`EnumInfo`、`FieldInfo`、`AliasInfo`、`PlaceholderInfo`、`StructInfo`、`FunctionInfo`、`ServletInfo`、`FileData`。

`FileData` 按种类存放这些声明。遍历顺序用各自的 `sequence`。

## 类型名

```text
TypeNameInfo
  name            带作用域的类型或模板名，含点号，不含参数
  is_parameter    默认 false
  arguments       特化参数，可嵌套
  constraints     空的 TypeConstraint 列表，检查逻辑还没做
```

- `list<int32>`：`name` 是 `list`，`arguments` 里是 `int32`。全名 `list<int32>` 由这两部分拼出来，不另存。
- `is_parameter` 只在声明占位符时为真，例如 `struct<K>` 里的 `K`。实例化后的 `list<int32>` 是确定类型，标志为假。
- 曾经加过 `Parameter`，已删除。泛型参数直接是 `StructInfo::parameters`，类型为 `vector<TypeNameInfo>`。
- `StructInfo::generic` 表示这个 struct 声明了类型参数。
- `FieldInfo::generic` 已删除。字段的泛型只看 `type_name`。
- `AnnotationInfo::typed` 已删除。没有 `<Type>` 时 `type_name.name` 为空。
- `FunctionInfo::return_type`、`AliasInfo::target`、`ServletInfo::bases`、`FieldInfo::type_name`、注解的 `type_name` 都是 `TypeNameInfo`。
- parser 里别名目标、servlet 基类、函数返回类型目前还只收下点号名字，不解析 `<>`。schema 已经能放下参数；解析补上之前 `arguments` 为空。

## 约束

`TypeConstraint` 是空结构，挂在 `TypeNameInfo::constraints` 上。

概念式检查（类似 concept，而不是 `PhysicalType` 白名单）还没做。`PlaceholderInfo` 里仍留着旧的 `constraint` 和 `allowed` 物理类型列表。

## 其他约定

- `EnumValueInfo::constexpr_ref` 仍是字符串。整数字面量时为空；否则保存源码里的整段引用，点号保留。绑定用整段字符串对 `full_name`。它不是类型，不改成 `TypeNameInfo`。
- servlet 基类不存在“同名不同作用域靠后续规则消歧”的问题。当前绑定就是拿写下的整段名字做精确查找，`shark.Read` 和 `other.Read` 是不同字符串。
- `Value` 仍是 `ValueKind` 加平行数组。曾讨论过 `std::variant` 更合适，还没有改。
