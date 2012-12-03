.. role:: unsure

**********************
第三章 LLVM IR代码生成
**********************

:原文: `Code generation to LLVM IR <http://llvm.org/docs/tutorial/LangImpl3.html>`_

本章简介
========

欢迎进入“\ :doc:`用LLVM开发新语言 <index>`\ ”教程的第三章。本章将介绍如何把第二章中构造的\ :doc:`抽象语法树 <chapter-2>`\ 转换成LLVM IR。从这一章起，我们就要正式接触LLVM了，你将亲眼见证LLVM的简单便捷：跟词法分析器和语法解析器比起来，LLVM IR代码生成部分的开发工作量根本就不值一提。 :-)

**请注意**\ ：本章及后续章节的代码必须用2.2以上版本的LLVM才能编译。LLVM 2.1及更早的版本都不行。另外也请注意选用与你所用的LLVM版本相配套的教程：如果你采用的是LLVM官方发行的版本，请参考版本自带的文档，在\ `llvm.org的历史版本汇总页`__\ 上可以找到各个版本的文档。

__ http://llvm.org/docs/tutorial/LangImpl3.html

代码生成的准备工作
==================

.. compound::

    在开始生成LLVM IR之前，还有一些准备工作要做。首先，给每个AST类添加一个虚函数\ ``Codegen``\ （code generation），用于实现代码生成：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 86-99
        :emphasize-lines: 5,13
        :append: ...

    每种AST节点的\ ``Codegen()``\ 方法负责生成该类型AST节点的IR代码及其他必要信息，生成的内容以LLVM ``Value``\ 对象的形式返回。LLVM用“\ ``Value``\ ”类表示“\ `静态一次性赋值（SSA，Static Single Assignment）`__\ 寄存器\ ”或“SSA值”。SSA值最为突出的特点就在于“固定不变”：SSA值经由对应指令运算得出后便固定下来，直到该指令再次执行之前都不可修改。详情请参考\ `Static Single Assignment`__\ ——这个概念并不难，习惯了就好。

__ http://en.wikipedia.org/wiki/Static_single_assignment_form
__ http://en.wikipedia.org/wiki/Static_single_assignment_form

除了在\ ``ExprAST``\ 类体系中添加虚方法以外，还可以利用\ `visitor模式`__\ 等其他方法来实现代码生成。再次强调，本教程不拘泥于软件工程实践层面的优劣：就当前需求而言，添加虚函数是最简单的方案。

__ http://en.wikipedia.org/wiki/Visitor_pattern

.. compound::

    其次，我们还需要一个“\ ``Error``\ ”方法，该方法与语法解析器里用到的报错函数类似，用于报告代码生成过程中发生的错误（例如引用了未经声明的参数）：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 355-356,351-353

    上述几个静态变量都是用于完成代码生成的。其中\ ``TheModule``\ 是LLVM中用于存放代码段中所有函数和全局变量的结构。从某种意义上讲，可以把它当作LLVM IR代码的顶层容器。

``Builder``\ 是用于简化LLVM指令生成的辅助对象。\ |IRBuilder|_\ 类模板的实例可用于跟踪当前插入指令的位置，同时还带有用于生成新指令的方法。

.. |IRBuilder| replace:: ``IRBuilder``
.. _IRBuilder: http://llvm.org/doxygen/IRBuilder_8h-source.html

``NamedValues``\ 映射表用于记录定义于当前作用域内的变量及与之相对应的LLVM表示（换言之，也就是代码的符号表）。在这一版的Kaleidoscope中，可引用的变量只有函数的参数。因此，在生成函数体的代码时，函数的参数就存放在这张表中。

有了这些，就可以开始进行表达式的代码生成工作了。注意，在生成代码之前必须先设置好\ ``Builder``\ 对象，指明写入代码的位置。现在，我们姑且假设已经万事俱备，专心生成代码即可。

表达式代码生成
==============

.. compound::

    为表达式节点生成LLVM代码的过程十分简单明了：连带注释只需区区45行代码便足以搞定全部四种表达式节点。首先是数值常量：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 357-359

    LLVM IR中的数值常量是由\ ``ConstantFP``\ 类表示的。在其内部，具体数值由\ ``APFloat``\ （Arbitrary Precision Float，可用于存储任意精度的浮点数常量）表示。这段代码说白了就是新建并返回了一个\ ``ConstantFP``\ 对象。值得注意的是，在LLVM IR内部，常量都只有一份，并且是共享的。因此，API往往会采用”\ ``foo:get(...)``\ “的形式而不是“\ ``new foo(...)``\ ”或“\ ``foo::Create(...)``\ ”。

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 361-365

.. compound::

    在LLVM中引用变量也很简单。在简化版的Kaleidoscope中，我们大可假设被引用的变量已经在某处被定义并赋值。实际上，位于\ ``NamedValues``\ 映射表中的变量只可能是函数的调用参数。这段代码首先确认给定的变量名是否存在于映射表中（如果不存在，就说明引用了未定义的变量）然后返回该变量的值。在后续章节中，我们还会对语言做进一步的扩展，让符号表支持“\ `循环归纳变量`__\ ”和\ `局部变量`__\ 。

    __ http://llvm.org/docs/tutorial/LangImpl5.html#for
    __ http://llvm.org/docs/tutorial/LangImpl7.html#localvars

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 367-383

    二元运算符的处理就比较有意思了。其基本思想是递归地生成代码，先处理表达式的左侧，再处理表达式的右侧，最后计算整个二元表达式的值。上述代码就\ ``opcode``\ 的取值用了一个简单的\ ``switch``\ 语句，从而为各种二元运算符创建出相应的LLVM指令。

在上面的例子中，LLVM的\ ``Builder``\ 类逐渐开始凸显出自身的价值。你只需想清楚该用哪些操作数（即此处的\ ``L``\ 和\ ``R``\ ）生成哪条指令（通过调用\ ``CreateFAdd``\ 等方法）即可，至于新指令该插入到什么位置，交给\ ``IRBuilder``\ 就可以了。此外，如果需要，你还可以给生成的指令指定一个名字。

LLVM的优点之一在于此处的指令名只是一个提示。举个例子，假设上述代码生成了多条“\ ``addtmp``\ ”指令，LLVM会自动给每条指令的名字追加一个自增的唯一数字后缀。指令的\ :unsure:`local value name`\ 完全是可选的，但它能大大提升dump出来的IR代码的可读性。

`LLVM指令`__\ 遵循严格的约束：例如，\ |add指令|_\ 的\ ``Left``\ 、\ ``Right``\ 操作数必须同属一个类型，结果的类型则必须与操作数的类型相容。由于Kaleidoscope中的值都是双精度浮点数，\ ``add``\ 、\ ``sub``\ 和\ ``mul``\ 指令的代码得以大大简化。

__ http://llvm.org/docs/LangRef.html#instref

.. |add指令| replace:: ``add``\ 指令
.. _add指令: http://llvm.org/docs/LangRef.html#i_add

.. compound::

    然而，LLVM要求\ |fcmp指令|_\ 的返回值类型必须是‘\ ``i1``\ ’（单比特整数）。问题在于Kaleidoscope只能接受\ ``0.0``\ 或\ ``1.0``\ 。为了弥合语义上的差异，我们给\ ``fcmp``\ 指令配上一条\ |uitofp指令|_\ 。这条指令会将输入的整数视作无符号数，并将之转换成浮点数。相应地，如果用的是\ |sitofp指令|_\ ，Kaleidoscope的‘\ ``<``\ ’运算符将视输入的不同而返回\ ``0.0``\ 或\ ``-1.0``\ 。

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 385-402

    函数调用的代码生成非常直截了当。上述代码开头的几行是在LLVM \ ``Module``\ 的符号表中查找函数名。如前文所述，LLVM ``Module``\ 是个容器，待处理的函数全都在里面。只要保证各函数的名字与用户指定的函数名一致，我们就可以利用LLVM的符号表替我们完成函数名的解析。

.. |fcmp指令| replace:: ``fcmp``\ 指令
.. |uitofp指令| replace:: ``uitofp``\ 指令
.. |sitofp指令| replace:: ``sitofp``\ 指令
.. _fcmp指令: http://llvm.org/docs/LangRef.html#i_fcmp
.. _uitofp指令: http://llvm.org/docs/LangRef.html#i_uitofp
.. _sitofp指令: http://llvm.org/docs/LangRef.html#i_sitofp

拿到待调用的函数之后，就递归地生成传入的各个参数的代码，并创建一条LLVM |call指令|_\ 。注意，LLVM默认采用本地的C调用规范，这样以来，就可以毫不费力地调用标准库中的“\ ``sin``\ ”、“\ ``cos``\ ”等函数了。

.. |call指令| replace:: ``call``\ 指令
.. _call指令: http://llvm.org/docs/LangRef.html#i_call

Kaleidoscope中的四种基本表达式的代码生成就介绍完了。尽情地添枝加叶去吧。去试试\ `LLVM语言参考`__\ 上的各种千奇百怪的指令，以当前的基本框架为基础，支持这些指令易如反掌。

__ http://llvm.org/docs/LangRef.html

函数的代码生成
==============



.. _chapter-3_full-code:

完整代码
========

.. literalinclude:: _includes/chapter-3_full.cpp
    :language: cpp

.. vim:ft=rst ts=4 sw=4 et
