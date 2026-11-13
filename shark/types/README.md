# Builtin types

Builtin types are field types. Numbers are the descriptor values and stay stable. Order is historical.

| Field type | C++ type | Wire |
| --- | --- | --- |
| `double` | `double` | 8 bytes |
| `float` | `float` | 4 bytes |
| `int64` | `int64_t` | varint, negative values take 10 bytes |
| `uint64` | `uint64_t` | varint |
| `int32` | `int32_t` | varint, negative values take 10 bytes |
| `fixed64` | `uint64_t` | 8 bytes |
| `fixed32` | `uint32_t` | 4 bytes |
| `bool` | `bool` | varint 0 or 1 |
| `string` | `string` | UTF-8, length-delimited |
| `group` | message | tag-delimited, deprecated |
| `message` | message | length-delimited |
| `bytes` | `string` | length-delimited |
| `uint32` | `uint32_t` | varint |
| `enum` | `int` | varint |
| `sfixed32` | `int32_t` | 4 bytes |
| `sfixed64` | `int64_t` | 8 bytes |
| `sint32` | `int32_t` | ZigZag varint |
| `sint64` | `int64_t` | ZigZag varint |
| `int8` | `int8_t` | 32-bit varint in this version |
| `uint8` | `uint8_t` | 32-bit varint in this version |
| `int16` | `int16_t` | 32-bit varint in this version |
| `uint16` | `uint16_t` | 32-bit varint in this version |

C++ types group the field types that share storage:

| C++ type | Field types |
| --- | --- |
| `INT32` | `int32`, `sint32`, `sfixed32` |
| `INT64` | `int64`, `sint64`, `sfixed64` |
| `UINT32` | `uint32`, `fixed32` |
| `UINT64` | `uint64`, `fixed64` |
| `DOUBLE` | `double` |
| `FLOAT` | `float` |
| `BOOL` | `bool` |
| `ENUM` | `enum` |
| `STRING` | `string`, `bytes` |
| `MESSAGE` | `message`, `group` |
| `INT8` | `int8` |
| `UINT8` | `uint8` |
| `INT16` | `int16` |
| `UINT16` | `uint16` |

`int8`, `uint8`, `int16`, and `uint16` each have their own C++ type. They do not share the `int32` / `uint32` slots. Memory is the real width. This version still writes a 32-bit varint, the same bytes as `int32` / `uint32`. Signed values are plain varint, not ZigZag. `-1` is the 10-byte sign-extended form. A value outside the width goes to the unknown fields and is not truncated. Packed repeated fields use the same varint packing as `int32` / `uint32`. `sint` and `sfixed` stay as they are.

Generated message fields already use `int8_t`, `uint8_t`, `int16_t`, and `uint16_t`.

## Required for int8, uint8, int16, uint16

`int64` already has each path below. These four types do not. Each item is required for the type to behave as a field type.

1. Packed repeated serialization reads the whole repeated field in one call, as `int64` reads `RepeatedField<int64_t>`, then writes the packed varints. `GetRepeatedFieldInternal` must exist for `int8_t`, `uint8_t`, `int16_t`, and `uint16_t`. The per-element write loop does not satisfy this.

2. Extensions store the value in their own slot, as `int64` stores `int64_t` and `RepeatedField<int64_t>`. Add `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, and a `RepeatedField` pointer for each. Remove the fold into `CPPTYPE_INT32` / `CPPTYPE_UINT32`. Allocation, clear, copy, swap, remove, and byte size use these slots.

3. Extension parse and serialize use that slot, as `int64` parses into `int64_t` and writes with `Int64Size`. A narrow value outside its width goes to the unknown fields. Bytes still match the 32-bit varint used by generated code. Packed repeated extensions use a `RepeatedField` of the real width.

4. Reflection reads and writes an extension through that slot, as `int64` does with `GetInt64`, `SetInt64`, and `AddInt64`. The 32-bit reflection accessors widen on read. `Set` and `Add` reject an out-of-width value and leave the extension unchanged.

5. `MapKey` stores the key in its own slot, as `int64` stores `int64_value_`. Compare, serialize, and byte size read `int8_t`, `uint8_t`, `int16_t`, or `uint16_t` and widen to 32 bits. The cpp type stays `INT8`, `UINT8`, `INT16`, or `UINT16`.

6. Map field operations branch on the cpp type, as they do for `int64`. `map_field.cc` must handle these four types for key and value. Serialize, merge, and compare then keep the entry.

7. Message copy, merge, and clear, `reflection_ops`, and dynamic-message construction and destruction handle the type, as they do for `int64`. Dynamic messages must construct and destroy `int8_t`, `uint8_t`, `int16_t`, `uint16_t`, and their `RepeatedField`s.

8. Descriptor map-key checks list the type as a legal key, as they list `int64`. Option assignment range-checks and stores the value, as it does for `int64`.

9. FieldMask copy, field comparison, and message differencer branch on the type, as they do for `int64`. Equal values compare equal. Different values compare different. FieldMask copies the value.

10. JSON resolves, parses, and prints the type, as it does for `int64`: type resolver, `DataPiece`, object writer, default-value writer, and proto stream read and write.

11. `parse_function_generator` emits parse and serialize branches for the type in every switch that already has `int64`. Existing `.skp` files stay until a regeneration is requested.

12. Tests fail when any required path above is wrong: extension singular and packed repeated round trips, out-of-width values left in the unknown fields, `MapKey` type and value, map entry key and value, JSON round trip, differencer equality, reflection copy, merge, and swap, and reflection packed bytes matching generated packed bytes.
