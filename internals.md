```
<bind> ::= <pattern> "=" <expr>;
<pattern> ::= <literal> | <variable> | <composit_pattern>;
<literal> ::= <integer> | <floating> | <string> | <symbol>;
<integer> ::= <digit_integer> | <octal_integer> | <hexadecimal_integer>;
<digit_integer> ::= [-+]?([1-9][0-9]*);
<octal_integer> ::= [-+]?(0[0-7]*);
<hexadecimal_integer> ::= [-+]?(0x[0-9a-fA-F]);


<composit_pattern> ::= <tuple>
```