Obscure things
==============

- `<->` swaps left and right operands (left-associative)
- `<~>` swaps left and right operands (right-associative)
- `<->` and `<~>` have equal precedence
- `[...]` is an asynchronous block; each of its lines are evaluated in parallel, but all threads are joined after closure of the block
- `{...}` is a synchronous block; each of its lines are evaluated sequentially
- `<...,...>` is an inline array (the bitshift operator does not *yet* exist)
- `(...)` is normal arithmetical expression grouping
- fractional (aka `frac`) datatype instead of floats
- all ints are bigints
- complex is a type prefix that is kind-of similar to array(2), except that multiplications use the complex conventions
- nested arrays are taken as matrices where the innermost array is taken as vertical. Example:

		[[1,2],[3,4]]*[5,6] =

		/ /1\ /3\ \ * /5\ = /23\
		\ \2/ \4/ /   \6/   \34/

		= [1,2]*5 + [3,4]*6
		= [5,10] + [18,24]
		= [23,34]

- string arithmetic is supported:
	- `"yolo++y"+1="olo++yy"`
	- `"yolo++y"+2="lo++yyo"`
	- `"yolo++y"-2="+yyolo+"`
	- `"yolo++y"*2="yolo++yyolo++y"`
	- `"yolo++y"*2.5="yolo++yyolo++yyolo"`
	- `"yolo++y"*2.2="yolo++yyolo++yy"` (length of string a = 7, so 2.2*7=15.4, rounded 15)
	- `"yolo++y"/2="yolo++y"*(1/2)="yolo"`
	- `"yolo++y"+"y++oloy"="yolo++++oloy"` (string concatenation)
	- `"yolo++y"-"y++oloy"="yolo"` (loop backwards over string b, deleting each character from string a until the beginning of a is reached)
- array arithmetic is supported, mostly vector operations. (##TODO more docs)

 - arithmetical operators:
 	- +
 	- -
 	- *
 	- /
 	- ^ (exponentiation) (on array: number of times after each other)
 	- &&
 	- ||
 	- ^^
 	- ? (on array/string: prefix, shuffle)
 	- ! (on array/string: prefix, sort) (##TODO but what if used as prefix not?)
