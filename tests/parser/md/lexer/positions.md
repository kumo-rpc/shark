# Lexer positions

## Line column

- type: shark-lexer
- positions: true

```input
a b\r\nc
```

```expect
1:1 BareWord "a"
1:2 Whitespace " "
1:3 BareWord "b"
1:4 Whitespace "\r\n"
2:1 BareWord "c"
2:2 EndOfStream ""
```

## Empty stream position

- type: shark-lexer
- positions: true

```input
```

```expect
1:1 EndOfStream ""
```

## Unclosed block position

- type: shark-lexer
- positions: true

```input
/*
```

```expect
1:1 ErrorMultilineCommentIsNotClosed "/*"
1:3 EndOfStream ""
```

## Unclosed block after text position

- type: shark-lexer
- positions: true

```input
a
/* not closed
   line 0\n
```

```expect
1:1 BareWord "a"
1:2 Whitespace "\n"
2:1 ErrorMultilineCommentIsNotClosed "/* not closed\n   line 0\n"
4:1 EndOfStream ""
```

## Nested block position

- type: shark-lexer
- positions: true

```input
/* outer /* inner
```

```expect
1:1 ErrorMultilineCommentIsNotClosed "/* outer /* inner"
1:18 EndOfStream ""
```

## Closed then still open position

- type: shark-lexer
- positions: true

```input
/* ok */ /* still open
```

```expect
1:1 Comment "/* ok */"
1:9 Whitespace " "
1:10 ErrorMultilineCommentIsNotClosed "/* still open"
1:23 EndOfStream ""
```

## Grammar head position

- type: shark-lexer
- positions: true

```input
// shot comment
/// long comment
/* multi lone comment
   line 0
   line 1
*

edition = "x2026"

import "ibex/schema.ibex"
import.cxx  "cstd"
import.java  "com.kumo.pub.algos"
import.go  "github.com/kumo-rpc/go-ibex"


package kumo.pub
package.java  "com.kumo.pub.algos"
package.go  "github.com/kumo-rpc/go-ibex"

struct MyStruct {
    optional int8 = 1;
}\n
```

```expect
1:1 Comment "// shot comment"
1:16 Whitespace "\n"
2:1 Comment "/// long comment"
2:17 Whitespace "\n"
3:1 ErrorMultilineCommentIsNotClosed "/* multi lone comment\n   line 0\n   line 1\n*\n\nedition = \"x2026\"\n\nimport \"ibex/schema.ibex\"\nimport.cxx  \"cstd\"\nimport.java  \"com.kumo.pub.algos\"\nimport.go  \"github.com/kumo-rpc/go-ibex\"\n\n\npackage kumo.pub\npackage.java  \"com.kumo.pub.algos\"\npackage.go  \"github.com/kumo-rpc/go-ibex\"\n\nstruct MyStruct {\n    optional int8 = 1;\n}\n"
23:1 EndOfStream ""
```
