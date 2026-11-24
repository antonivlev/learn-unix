## Functions

Arguments are passed **by value**. Function gets copies of args as temp variables.

Except! Arrays. When array is passed in _address_ of the array is passed in.

```c
// why this is ok
void f(char buf[]) {}

// but for more dims SIZE is needed
void f(char buf[][SIZE]) {}
```

Because multi-dim arrays lie in a line in memory. E.g

```
list = [
  [a b c d]
  [e f g h]
  [x y z p]
]
==>
mem_list = [a b c d e f g h x y z p]
```

So to get an element, e.g. `list[1][2] = g = mem_list[6]`

```
mem_i = (SIZE = 4) * 1 + 2
```

you need SIZE
