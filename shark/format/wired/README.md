# Protobuf wire format

A message is a sequence of fields. A field is a tag plus a payload. The tag encodes the field number and the wire type.

## Message

| Field 1 | Field 2 | Field 3 | ... |
| --- | --- | --- | --- |
| Tag + Payload | Tag + Payload | Tag + Payload | ... |

| Property | Meaning |
| --- | --- |
| Message header | none |
| Message length | none; the caller or the outer framing supplies the bounds |
| Field count | none |
| Field order | encode order; order is not field identity |
| Empty message | empty byte sequence |

A field with no encoded value produces no bytes. A missing field and a field with no encoded value are the same bytes.

## Field

| Tag | Payload |
| --- | --- |
| Varint | determined by the wire type |

The tag value is 32 bits. The low 3 bits are the wire type. The high 29 bits are the field number. A field number is at most `2^29 - 1`.

| Bits | Width | Meaning |
| --- | --- | --- |
| 31..3 | 29 | field number |
| 2..0 | 3 | wire type |

```text
tag = (field_number << 3) | wire_type
```

Those 32 bits are then encoded as a varint, so the tag on the wire is 1 to 5 bytes. Field numbers 1 through 15 fit in one byte.

## Wire type

The wire type selects how to read the payload. The length of a length-delimited payload is itself a varint.

| Wire type | Name | Payload | Used by |
| ---: | --- | --- | --- |
| 0 | Varint | variable-length integer | integers, `bool`, `enum` |
| 1 | Fixed64 | 8 bytes | `double`, `fixed64`, `sfixed64` |
| 2 | Length-delimited | length + data | `string`, `bytes`, `message`, packed repeated |
| 3 | Start group | legacy | legacy group |
| 4 | End group | legacy | legacy group |
| 5 | Fixed32 | 4 bytes | `float`, `fixed32`, `sfixed32` |

Ordinary data uses 0, 1, 2, and 5.

## Type mapping

| Logical type | Wire type | Payload |
| --- | ---: | --- |
| `int8` / `int16` / `int32` | 0 | varint, sign-extended when negative |
| `uint8` / `uint16` / `uint32` | 0 | varint |
| `int64` | 0 | varint, sign-extended when negative |
| `uint64` | 0 | varint |
| `sint32` / `sint64` | 0 | ZigZag, then varint |
| `bool` | 0 | varint, `00` or `01` |
| `enum` | 0 | varint |
| `fixed32` / `sfixed32` | 5 | 4 bytes, little-endian |
| `float` | 5 | IEEE-754, 4 bytes, little-endian |
| `fixed64` / `sfixed64` | 1 | 8 bytes, little-endian |
| `double` | 1 | IEEE-754, 8 bytes, little-endian |
| `string` / `bytes` | 2 | length + data |
| nested `message` | 2 | length + child message |

`int8` and `int16` use the same bytes as `int32`. `uint8` and `uint16` use the same bytes as `uint32`. They are not separate wire types.

## Repeated, packed, and map

Repeated. Each element is its own field:

```text
Tag + Value
Tag + Value
Tag + Value
```

Packed repeated. One field holds every element:

```text
Tag + Length + Value + Value + Value
```

Map. A repeated entry message:

```text
Map field
    Entry message
        key
        value
```

`string`, `bytes`, `message`, and groups are not packed.

## Varint

Each byte stores 7 data bits. The high bit is 1 when another byte follows, and 0 on the last byte. The low 7-bit group is written first.

```text
1   -> 01
127 -> 7f
128 -> 80 01
150 -> 96 01
```

Small integers take fewer bytes. Large integers take more.

`int32 id = 1` with value 150: field number 1, wire type 0, tag `(1 << 3) | 0 = 0x08`. The field is `08 96 01`.

A negative `int32`, `int64`, `int8`, or `int16` is sign-extended to 64 bits before the varint, so `-1` is 10 bytes:

```text
ff ff ff ff ff ff ff ff ff 01
```

## ZigZag

`sint32` and `sint64` still use wire type 0. The signed value is mapped to an unsigned value, then written as a varint:

```text
signed integer -> ZigZag -> unsigned integer -> varint
```

32-bit ZigZag is `(n << 1) ^ (n >> 31)`. 64-bit ZigZag is `(n << 1) ^ (n >> 63)`. `sint32` value `-1` is the byte `01`.

## Fixed widths and byte order

Fixed32 and Fixed64 are little-endian. `float` and `double` are IEEE-754 in that byte order.

Field 1, wire type 5: tag `(1 << 3) | 5 = 0x0d`, then 4 bytes.

Field 1, wire type 1: tag `(1 << 3) | 1 = 0x09`, then 8 bytes.

## Length-delimited

```text
Tag
Length
Data
```

`string name = 1`, value `"ab"`:

```text
0a 02 61 62
```

`0a` is the tag, `02` is the length, `61 62` is the data.

A nested message is the same form. The data is a message body. It has no wire type of its own.

```proto
message User {
    int32 id = 1;
    string name = 2;
}

message Request {
    User user = 1;
}
```

`user` is wire type 2: tag, message length, message body.

## Repeated and packed bytes

```proto
repeated int32 ids = 4;
```

Values 1, 2, 3, unpacked. Field 4, wire type 0, tag `20`:

```text
20 01
20 02
20 03
```

Packed. Field 4, wire type 2, tag `22`:

```text
22        tag
03        length
01 02 03  values
```

`int8`, `int16`, `uint8`, and `uint16` pack the same varints as `int32` and `uint32`.

## Map bytes

```proto
map<string, int32> values = 1;
```

is a repeated message:

```proto
message Entry {
    string key = 1;
    int32 value = 2;
}

repeated Entry values = 1;
```

Each entry is one length-delimited field. Inside it, field 1 is the key and field 2 is the value.

## Unknown fields

Unknown fields are parser behavior. The wire type is enough to skip a payload:

```text
0  read a varint
1  skip 8 bytes
2  read a length, then skip that many bytes
5  skip 4 bytes
```

An implementation may keep those bytes and write them back.

## Legacy groups

Wire type 3 starts a group. Wire type 4 ends it. Both tags use the same field number. The bytes between them are a message body. Groups remain legal. A current schema does not use them.
