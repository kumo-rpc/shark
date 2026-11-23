# Lexer strings

## Quoted string and identifiers

- type: shark-lexer

```input
'hello''world' "x" `y`
```

```expect
StringLiteral "'hello''world'"
Whitespace " "
QuotedIdentifier "\"x\""
Whitespace " "
QuotedIdentifier "`y`"
EndOfStream ""
```

## Unclosed single quote

- type: shark-lexer

```input
'abc
```

```expect
ErrorSingleQuoteIsNotClosed "'abc"
EndOfStream ""
```

## Unclosed double quote

- type: shark-lexer

```input
"abc
```

```expect
ErrorDoubleQuoteIsNotClosed "\"abc"
EndOfStream ""
```

## Unclosed back quote

- type: shark-lexer

```input
`abc
```

```expect
ErrorBackQuoteIsNotClosed "`abc"
EndOfStream ""
```

## Unicode single quotes

- type: shark-lexer

```input
‘hi’
```

```expect
StringLiteral "\xE2\x80\x98hi\xE2\x80\x99"
EndOfStream ""
```

## Unicode double quotes

- type: shark-lexer

```input
“id”
```

```expect
QuotedIdentifier "\xE2\x80\x9Cid\xE2\x80\x9D"
EndOfStream ""
```
