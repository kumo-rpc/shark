# Lexer comments

## Line comment

- type: shark-lexer

```input
a-- c
b
```

```expect
BareWord "a"
Comment "-- c"
Whitespace "\n"
BareWord "b"
EndOfStream ""
```

## Block comment

- type: shark-lexer

```input
a/* x */b
```

```expect
BareWord "a"
Comment "/* x */"
BareWord "b"
EndOfStream ""
```

## Unclosed block at end

- type: shark-lexer

```input
/*
```

```expect
ErrorMultilineCommentIsNotClosed "/*"
EndOfStream ""
```

## Hash comment

- type: shark-lexer

```input
# hello
```

```expect
Comment "# hello"
EndOfStream ""
```

## Hash without space

- type: shark-lexer

```input
#hello
```

```expect
Error "#"
BareWord "hello"
EndOfStream ""
```

## Unclosed block after text

- type: shark-lexer

```input
a
/* not closed
   line 0\n
```

```expect
BareWord "a"
Whitespace "\n"
ErrorMultilineCommentIsNotClosed "/* not closed\n   line 0\n"
EndOfStream ""
```

## Nested block is not closed

- type: shark-lexer

```input
/* outer /* inner
```

```expect
ErrorMultilineCommentIsNotClosed "/* outer /* inner"
EndOfStream ""
```

## Closed then still open

- type: shark-lexer

```input
/* ok */ /* still open
```

```expect
Comment "/* ok */"
Whitespace " "
ErrorMultilineCommentIsNotClosed "/* still open"
EndOfStream ""
```
