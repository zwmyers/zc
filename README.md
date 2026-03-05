experiment in interpreters<br>
lexical scoping, full ast-walk<br>

currently supports:<br>
strings, ints, arrays can all be printed with print(args);<br>
len(args) with array arg gives int value of length<br>
array(size) with int arg gives array files with 0s<br>
swap(arr, i, j) with array, int, int args swaps elements in array at indices i and j<br>

TODO<BR>
break and continue -> for loops<br>
fix memory leaks (copying? arena?)
