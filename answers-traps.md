# Answers-traps

## Which registers contain arguments to functions? For example, which register holds 13 in main's call to printf? 
放到参数寄存`a0`~`a7`中，`printf()`中`a2`保存`13`。

## Where is the call to function f in the assembly code for main? Where is the call to g? (Hint: the compiler may inline functions.) 
编译器采用了内联函数，`main()`中`li a1, 12`相当于调用了`f(8)+1`。

## At what address is the function printf located? 
`0x628`。

## What value is in the register ra just after the jalr to printf in main? 
`jarl 1528(ra)` <=> `jarl ra, 1528(ra)`，跳转后，R[ra]=0x34 + 4 = 0x38。

## Little and Big endian.
>   Run the following code.

        ```c
        unsigned int i = 0x00646c72;
        printf("H%x Wo%s", 57616, &i);
        ```
    What is the output? Here's an ASCII table that maps bytes to characters.

    The output depends on that fact that the RISC-V is little-endian. If the RISC-V were instead big-endian what would you set i to in order to yield the same output? Would you need to change 57616 to a different value?

+ 输出结果：He110 World
+ i = 0x726c6400; 
+ 不需要更改`57616`为其他数值，字面量会由编译器自己处理。


## In the following code, what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?

    ```c
	printf("x=%d y=%d", 3);
    ```

取决于`a2`之前保存的值。


      


