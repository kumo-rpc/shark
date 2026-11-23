# Lexer tokens

## Empty

- type: shark-lexer

```input
```

```expect
EndOfStream ""
```

## Simple select

- type: shark-lexer

```input
SELECT a FROM t
```

```expect
BareWord "SELECT"
Whitespace " "
BareWord "a"
Whitespace " "
BareWord "FROM"
Whitespace " "
BareWord "t"
EndOfStream ""
```

## Numbers

- type: shark-lexer

```input
123 1.5 1e10 0x1f 1_000
```

```expect
Number "123"
Whitespace " "
Number "1.5"
Whitespace " "
Number "1e10"
Whitespace " "
Number "0x1f"
Whitespace " "
Number "1_000"
EndOfStream ""
```

## Dot versus float chained

- type: shark-lexer

```input
x.1.1
```

```expect
BareWord "x"
Dot "."
Number "1"
Dot "."
Number "1"
EndOfStream ""
```

## Leading dot number

- type: shark-lexer

```input
.1
```

```expect
Number ".1"
EndOfStream ""
```

## Operators

- type: shark-lexer

```input
( ) + - * / % < > = <=> != <> -> :: || |> @ @@ ?
```

```expect
OpeningRoundBracket "("
Whitespace " "
ClosingRoundBracket ")"
Whitespace " "
Plus "+"
Whitespace " "
Minus "-"
Whitespace " "
Asterisk "*"
Whitespace " "
Slash "/"
Whitespace " "
Percent "%"
Whitespace " "
Less "<"
Whitespace " "
Greater ">"
Whitespace " "
Equals "="
Whitespace " "
Spaceship "<=>"
Whitespace " "
NotEquals "!="
Whitespace " "
NotEquals "<>"
Whitespace " "
Arrow "->"
Whitespace " "
DoubleColon "::"
Whitespace " "
Concatenation "||"
Whitespace " "
PipeOperator "|>"
Whitespace " "
At "@"
Whitespace " "
DoubleAt "@@"
Whitespace " "
QuestionMark "?"
EndOfStream ""
```

## Brackets comma semicolon

- type: shark-lexer

```input
[]{},;
```

```expect
OpeningSquareBracket "["
ClosingSquareBracket "]"
OpeningCurlyBrace "{"
ClosingCurlyBrace "}"
Comma ","
Semicolon ";"
EndOfStream ""
```

## Minus

- type: shark-lexer

```input
-
```

```expect
Minus "-"
EndOfStream ""
```

## Arrow

- type: shark-lexer

```input
->
```

```expect
Arrow "->"
EndOfStream ""
```

## Colon

- type: shark-lexer

```input
:
```

```expect
Colon ":"
EndOfStream ""
```

## Pipe

- type: shark-lexer

```input
|
```

```expect
PipeMark "|"
EndOfStream ""
```

## Single exclamation

- type: shark-lexer

```input
!
```

```expect
ErrorSingleExclamationMark "!"
EndOfStream ""
```

## Vertical delimiter

- type: shark-lexer

```input
\G
```

```expect
VerticalDelimiter "\\G"
EndOfStream ""
```

## Bad backslash

- type: shark-lexer

```input
\X
```

```expect
Error "\\"
BareWord "X"
EndOfStream ""
```

## Dollar

- type: shark-lexer

```input
$
```

```expect
DollarSign "$"
EndOfStream ""
```

## Here document

- type: shark-lexer

```input
$tag$hello$tag$
```

```expect
HereDoc "$tag$hello$tag$"
EndOfStream ""
```

## Hex string

- type: shark-lexer

```input
x'abc'
```

```expect
StringLiteral "x'abc'"
EndOfStream ""
```

## Bin string

- type: shark-lexer

```input
b'01'
```

```expect
StringLiteral "b'01'"
EndOfStream ""
```

## Unclosed hex string

- type: shark-lexer

```input
x'ab
```

```expect
ErrorSingleQuoteIsNotClosed "x'ab"
EndOfStream ""
```
