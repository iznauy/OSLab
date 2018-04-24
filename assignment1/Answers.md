1. `org 0x7c00`作用

   告诉编译器，代码将加载到`0x7c00`，遇到相对地址的时候就会自动加上`0x7c00`，这样就直接得到了绝对址。

2. 为什么`boot.bin`要放在第一个扇区，为什么直接复制不行

   BIOS程序检测磁盘0面0磁道1扇区，如果以`0xaa55`结尾，那么任务是引导扇区，加载到`0x7c00`处进行执行。

普通的移动、复制都是基于文件系统的，文件系统是一个逻辑上的概念，而引导扇区是磁盘0面0磁道1扇区，是一个物理概念，这个扇区在文件系统中根本不可见。

3. `loader`的作用

加载操作系统内核；进入保护模式；内存分页

5. `times 510-($-$$) db 0`

   `$`表示这行代码的逻辑地址，`$$`表示该段的开始地址。

至于510，是因为引导扇区长度必须为512字节，而最后两个字节是固定的，所以就是510。

可以使用rep操作符

```assembly
%rep 510-($-$$)
	db 0
%endrep
```

6. 解释命令

   顺次存储`w, o, r, d, \0`进入内存中，事实上相当于存储了一个字符串，

至于最后的`\0`，是要标识字符串的结尾。

7. `L1 db 0; L2 dw 1000`，`L1, L2`是连续存储的嘛？

   是的，`L2`就在`L1`之后。

8. `mov [L6], 1`是否正确？

   不对，MOV指令要求源操作数和目的操作数的类型必须要一致，也就是同时为字或者字节。

9. 如何处理输入输出

   通过系统调用来处理输入输出。

   如在X64下，当`rax`中值为0的时候，表示读取一个字节的输入，之后使用指令`syscall`就可以读取系统输入。

10. 如何保存前一次运算结果

    绝大多数运算结果都会保存在通用寄存器中；

    一部分运算的结果(主要是逻辑运算)会保存在程序状态字(标志寄存器)中；

    此外，栈也可以保存运算结果

11. 有哪些段寄存器？

    - 代码段寄存器(CS)
    - 数据段寄存器(DS)
    - 堆栈段寄存器(SS)
    - 附加段寄存器(ES)

12. 8086、8088存储单元的物理地址长，CPU总线的数量，可以直接寻址的物理地址空间

    20， 20， 1MB

    8086/8088地址总线都是20位；外部数据总线宽度：8086是16位，8088是8位。内部数据总线宽度都是16位。

    寻址时候：物理地址 = 段址 * 16 + 偏移量

13. 寄存器的寻址方式

    - 立即寻址 -> 常常用于给寄存器和存储单元赋值时候(就是立即数)
    - 寄存器寻址 -> `mov si, ax`
    - 直接寻址 -> 直接把地址给出来，放在方括号里 `mov ax, [1234h]`
    - 寄存器间接寻址 -> 寄存器中存放的数据的地址 `mov bx, [di]`
    - 寄存器相对寻址 -> 寄存器加操作数 `mov bx, [di + 100h]`
    - 基址+变址寻址 -> 常常用于循环结构 `mov bx, [bx + si]`
    - 相对基址+变址寻址 -> 在多层循环中会用 `mov ax, [bx + si + 200h]`

14. 常用指令

    - mov -> 传送指令，把一个字/字节的从源操作数送到目的操作数
      - 必须要是同类型的
      - 除了串操作指令，源操作数和目的操作数要一致
    - lea 地址传送指令
      - lea reg, oprd -> 把oprd的有效地址 oprd必须是一个存储器操作数
      - 必须是一个存储器操作数
    - Push，Pop
    - 加减运算
    - 逻辑运算
    - 跳转指令

15. 参数传递方式

    - 用寄存器传递参数
    - 用约定的存储单元传递参数 -> 放在静态数据区里的某块区域
    - 利用堆栈传递参数 -> 事实上就C、C++传递参数的方式 `push, pop`
    - CALL后续区传递参数