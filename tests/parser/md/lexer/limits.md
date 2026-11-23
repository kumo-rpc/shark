# Lexer limits

## Max query size

- type: shark-lexer
- max-query-size: 3

```input
abcdef
```

```expect
ErrorMaxQuerySizeExceeded "abcdef"
EndOfStream ""
```

## Non breaking space

- type: shark-lexer

```input
a\xC2\xA0b
```

```expect
BareWord "a"
Whitespace "\xC2\xA0"
BareWord "b"
EndOfStream ""
```

## Unicode minus

- type: shark-lexer

```input
\xE2\x88\x92
```

```expect
Minus "\xE2\x88\x92"
EndOfStream ""
```
