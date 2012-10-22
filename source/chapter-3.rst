.. role:: unsure

**********************
第三章 LLVM IR代码生成
**********************

:原文: `Code generation to LLVM IR <http://llvm.org/docs/tutorial/LangImpl3.html>`_

本章简介
========

欢迎进入“\ :doc:`用LLVM开发新语言 <index>`\ ”教程的第三章。本章将要介绍的是如何将第二章中构造的\ :doc:`抽象语法树 <chapter-2>`\ 转换成LLVM IR。从这一章起，我们就要正式开始接触LLVM了，你将亲眼目睹LLVM的简单便捷。LLVM IR代码生成的开发工作量跟词法分析器和语法解析器比起来简直就是小巫见大巫。 :-)

**请注意**\ ：本章及后续章节的代码必须用2.2以上版本的LLVM才能编译。LLVM 2.1及更早的版本都不行。另外也请注意选用与你的LLVM版本相配套的教程：如果你用的是LLVM官方发行的版本，请参考对应版本自带的文档，在\ `llvm.org的历史版本汇总页`__\ 上也能找到各个版本的文档。

__ http://llvm.org/docs/tutorial/LangImpl3.html

代码生成的准备工作
==================

.. compound::

    在生成LLVM IR之前，还有一些准备工作要做。首先，需要给每个AST类添加一个负责代码生成的虚函数\ ``Codegen``\ （code generation）：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 86-99
        :emphasize-lines: 5,13
        :append: ...

    ``Codegen()``\ 方法的职能就是为对应的AST节点生成IR以及和IR相关的各种必要内容，该函数返回的是一个LLVM ``Value``\ 对象。“\ ``Value``\ ”是LLVM用来表示“\ `静态单次赋值（SSA，Static Single Assignment）`__\ 寄存器”或“SSA值”的类。SSA值最突出的特点在于，它们的值一旦被相关指令计算出来，就一直保持不变，直至指令再次执行（如果有机会的话）。换句话说，SSA值是“恒定不变”的。详情请参考\ `Static Single Assignment`__\ ——理解之后就会发现，这个概念还是挺自然的。

__ http://en.wikipedia.org/wiki/Static_single_assignment_form
__ http://en.wikipedia.org/wiki/Static_single_assignment_form

除了在\ ``ExprAST``\ 类体系中添加虚方法以外，还可以利用\ `visitor模式`__\ 等其他方法来实现代码生成。再次强调，本教程不拘泥于软件工程实践层面的优劣：就我们当前的需求而言，添加虚函数是最简单的方案。

__ http://en.wikipedia.org/wiki/Visitor_pattern

.. compound::

    其次，我们还需要一个“\ ``Error``\ ”方法，该函数与语法解析器里用到的报错函数类似，用于报告在代码生成过程中发现的错误（例如引用了未经声明的参数）：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 355-356,351-353

    几个静态变量都是用于代码生成过程的。其中的\ ``TheModule``\ 是LLVM中用于存放某段代码中的所有函数和全局变量的结构。从某种程度上讲，它是LLVM IR最顶层的代码容器。

``Builder``\ 对象是一个用于简化LLVM指令生成过程的辅助对象。\ |IRBuilder|_\ 类模板的实例可用于跟踪当前的指令插入位置，同时还提供了用于创建新指令的方法。

.. |IRBuilder| replace:: ``IRBuilder``
.. _IRBuilder: http://llvm.org/doxygen/IRBuilder_8h-source.html

``NamedValues``\ 映射表则用于跟踪在当前作用域内定义的变量及与其相对应的LLVM表示（也就是代码的符号表）。在这一版的Kaleidoscope中，唯一可引用的变量就是函数的参数。因此，在生成函数体的代码的同时，函数的参数就存放在这张表中。

有了这些作为基础，我们就可以开始讨论表达式的代码生成了。注意，在生成代码之前必须先设置好\ ``Builder``\ 对象，指明代码的写入位置。现在，我们姑且假设一切准备就绪，只需生成代码即可。

表达式代码生成
==============

.. compound::

    为表达式节点生成LLVM代码的过程非常简单明了：连带注释只需区区45行代码便足以处理全部四种表达式节点。首先是数值常量：

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 357-359

    LLVM IR中的数值常量是由\ ``ConstantFP``\ 类来表示的。在其内部，具体数值由\ ``APFloat``\ （Arbitrary Precision Float，可用于存储任意精度的浮点数常量）表示。这段代码基本上就是新建了一个\ ``ConstantFP``\ 并将之返回。值得注意的是，在LLVM IR内部，常量都只有一份，并且是共享的。因此，API中采用的往往是“\ ``foo:get(...)``\ ”，而不是“\ ``new foo(...)``\ “或“\ ``foo::Create(...)``\ ”。

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 361-365

.. compound::

    用LLVM引用变量也十分简单。在简化版的Kaleidoscope中，我们只管假设被引用的变量已经在某处定义并赋值。实际上，位于\ ``NamedValues``\ 映射表中的变量只可能是函数的调用参数。这段代码会确认映射表中的确包含给定的变量名（如果不存在，就说明引用了未定义的变量）并返回该变量的值。在后续章节中，我们还会对语言做进一步的扩展，让符号表能够支持“\ `循环归纳变量`__\ ”和\ `局部变量`__\ 。

    __ http://llvm.org/docs/tutorial/LangImpl5.html#for
    __ http://llvm.org/docs/tutorial/LangImpl7.html#localvars

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 367-383

    二元运算符的处理就比较有意思了。基本的思想是递归地生成代码，先处理表达式的左侧，再处理表达式的右侧，最后再计算整个二元表达式的值。这段代码简单地通过\ ``switch``\ 为不同的二元运算符创建不同的LLVM指令。

在上面的例子中，LLVM ``Builder``\ 类的价值开始体现出来了。\ ``IRBuilder``\ 知道应该将新创建的指令插到何处，你只需（调用\ ``CreateFAdd``\ 等方法）指定用什么操作数（即这里的\ ``L``\ 和\ ``R``\ ）创建什么指令就可以了。此外，根据需要，你还可以为生成指令指定一个名字。

One nice thing about LLVM is that the name is just a hint. For instance, if the code above emits multiple "``addtmp``" variables, LLVM will automatically provide each one with an increasing, unique numeric suffix. Local value names for instructions are purely optional, but it makes it much easier to read the IR dumps.

LLVM的优点之一是指令的名字仅仅只是一个提示。例如，如果上面的代码生成了多条“\ ``addtmp``\ ”指令，LLVM会自动给它们追加一个不断自增的、唯一的数字后缀。指令的\ :unsure:`local value name`\ 不是必须的，但它会让

`LLVM指令`__\ 都必须遵循严格的限制：例如，\ |add指令|_\ 的\ ``Left``\ 和\ ``Right``\ 操作数必须同属一个类型，结果的类型则必须与操作数的类型相容。由于在Kaleidoscope中只有双精度浮点数，生成\ ``add``\ 、\ ``sub``\ 和\ ``mul``\ 指令的代码简化了许多。

__ http://llvm.org/docs/LangRef.html#instref

.. |add指令| replace:: ``add``\ 指令
.. _add指令: http://llvm.org/docs/LangRef.html#i_add

.. compound::

    On the other hand, LLVM specifies that the `fcmp instruction`__ always returns an '``i1``' value (a one bit integer). The problem with this is that Kaleidoscope wants the value to be a 0.0 or 1.0 value. In order to get these semantics, we combine the fcmp instruction with a `uitofp instruction`__. This instruction converts its input integer into a floating point value by treating the input as an unsigned value. In contrast, if we used the `sitofp instruction`__, the Kaleidoscope '<' operator would return 0.0 and -1.0, depending on the input value.

    .. literalinclude:: _includes/chapter-3_full.cpp
        :language: cpp
        :lines: 385-402

    Code generation for function calls is quite straightforward with LLVM. The code above initially does a function name lookup in the LLVM Module's symbol table. Recall that the LLVM Module is the container that holds all of the functions we are JIT'ing. By giving each function the same name as what the user specifies, we can use the LLVM symbol table to resolve function names for us.

__ http://llvm.org/docs/LangRef.html#i_fcmp
__ http://llvm.org/docs/LangRef.html#i_uitofp
__ http://llvm.org/docs/LangRef.html#i_sitofp

Once we have the function to call, we recursively codegen each argument that is to be passed in, and create an LLVM `call instruction`__. Note that LLVM uses the native C calling conventions by default, allowing these calls to also call into standard library functions like "sin" and "cos", with no additional effort.

__ http://llvm.org/docs/LangRef.html#i_call

This wraps up our handling of the four basic expressions that we have so far in Kaleidoscope. Feel free to go in and add some more. For example, by browsing the `LLVM language reference`__ you'll find several other interesting instructions that are really easy to plug into our basic framework.

__ http://llvm.org/docs/LangRef.html

.. _chapter-3_full-code:

完整代码
========

.. literalinclude:: _includes/chapter-3_full.cpp
    :language: cpp

.. vim:ft=rst ts=4 sw=4 et
