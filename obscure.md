Obscure things
==============

- `<->` swaps left and right operands (left-associative)
- `<~>` swaps left and right operands (right-associative)
- `<->` and `<~>` have equal precedence
- `[...]` is an asynchronous block; each of its lines are evaluated in parallel and the array of results is returned
- `{...}` is a synchronous block; each of its lines are evaluated sequentially and the result is returned
- `<...,...>` is an inline array (the bitshift operator does not *yet* exist)
- `(...)` is normal arithmetical expression grouping
- fractional (aka `frac`) datatype instead of floats
- all ints are bigints
- ***?*** all numbers are complex
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
 	- + (on bitset: xor)
 	- -
 	- * (on bitset: and)
 	- /
 	- ^ (exponentiation) (on array: number of times after each other)
 	- ! (postfix; factorial) (on array: item product)
 	- &&
 	- ||
 	- ^^
 	- | (only on bitset)
 	- ? (on array/string: prefix, shuffle)
 	- ! (on array/string: prefix, sort) (but what if used as prefix not? ##TODO)
