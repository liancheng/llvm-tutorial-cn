.. _chapter-2:

**************************
第二章 实现语法分析器和AST
**************************

:原文: `Implementing a Parser and AST <http://llvm.org/docs/tutorial/LangImpl2.html>`_

本章简介
========

欢迎进入“用LLVM开发新语言”教程的第二章。在本章中，我们将以第一章中开发的词法分析器为基础，为Kaleidoscope语言开发一个完整的\ `语法解析器`__\ 。搞定语法解析器之后，我们就开始定义并构造\ `抽象语法树`__\ （AST，Abstract Syntax Tree）。

__ http://en.wikipedia.org/wiki/Parsing
__ http://en.wikipedia.org/wiki/Abstract_syntax_tree

在解析Kaleidoscope的语法时，我们将综合运用\ `递归下降解析`__\ 和\ `运算符优先级解析`__\ 两种技术（后者用于解析二元表达式，前者用于其他部分）。正式开始解析之前，不妨先来看一看解析器的输出：抽象语法树。

__ http://en.wikipedia.org/wiki/Recursive_descent_parser
__ http://en.wikipedia.org/wiki/Operator-precedence_parser

抽象语法树（AST）
=================

.. compound::

    抽象语法树的作用在于牢牢抓住程序的脉络，从而方便编译过程的后续环节（如代码生成）对程序进行解读。AST就是开发者为语言量身定制的一套模型，基本上语言中的每种结构都与一种AST对象相对应。Kaleidoscope语言中的语法结构包括表达式、函数原型和函数对象。我们不妨先从表达式入手：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 74-89

    上述代码定义了基类\ ``ExprAST``\ 和一个用于表示数值常量的子类。其中子类\ ``NumberExprAST``\ 将数值常量的值存放在成员变量中，以备编译器后续查询。

.. compound::

    直到目前为止，我们还只搭出了AST的架子，尚未定义任何能够体现AST实用价值的成员方法。例如，只需添加一套虚方法，我们就可以轻松实现代码的格式化打印。以下定义了Kaleidoscope语言最基本的部分所要用到的其他各种表达式的AST节点：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 91-114

    我们（特意）将这几个类设计得简单明了：\ ``VariableExprAST``\ 用于保存变量名，\ ``BinaryExprAST``\ 用于保存运算符（如“\ ``+``\ ”），\ ``CallExprAST``\ 用于保存函数名和用作参数的表达式列表。这样设计AST有一个优势，那就是我们无须关注语法便可直接抓住语言本身的特性。注意这里还没有涉及到二元运算符的优先级和词法结构等问题。

.. compound::

    定义完毕这几种表达式节点，就足以描述Kaleidoscope语言中的几个最基本的结构了。由于我们还没有实现条件控制流程，它还不算图灵完备；后续还会加以完善。接下来，还有两种结构需要描述，即函数的接口和函数本身：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 116-136

    在Kaleidoscope中，函数的类型是由参数的个数决定的。由于所有的值都是双精度浮点数，没有必要保存参数的类型。在更强大、更实用的语言中，\ ``ExprAST``\ 类多半还会需要一个类型字段。

有了这些作为基础，我们就可以开始解析Kaleidoscope的表达式和函数体了。

语法解析器基础
==============

.. compound::

    开始构造AST之前，先要准备好用于构造AST的语法解析器。说白了，就是要利用语法解析器把“\ ``x+y``\ ”这样的输入（由词法分析器返回的三个语元）分解成由下列代码生成的AST：

    .. literalinclude:: _includes/chapter-2_sample-1.cpp
        :language: cpp

    为此，我们先定义几个辅助函数：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 142-148

    这段代码以词法分析器为中心，实现了一个简易的语元缓冲，让我们能够预先读取词法分析器将要返回的下一个语元。在我们的语法解析器中，所有函数都将\ ``CurTok``\ 视作当前待解析的语元。

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 165-168

    这三个用于报错的辅助函数也很简单，我们的语法解析器将用它们来处理解析过程中发生的错误。这里采用的错误恢复策略并不妥当，对用户也不怎么友好，但对于教程而言也就够用了。示例代码中各个函数的返回值类型各不相同，有了这几个函数，错误处理就简单了：它们的返回值统统是\ ``NULL``\ 。

准备好这几个辅助函数之后，我们就开始实现第一条语法规则：数值常量。

解析基本表达式
==============

.. compound::

    之所以先从数值常量下手，是因为它最简单。Kaleidoscope语法中的每一条生成规则（production），都需要一个对应的解析函数。对于数值常量，就是：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 206-211

    这个函数很简单：调用它的时候，当前待解析语元只能是\ ``tok_number``\ 。该函数用刚解析出的数值构造出了一个\ ``NumberExprAST``\ 节点，然后令词法分析器继续读取下一个语元，最后返回构造的AST节点。

.. compound::

    这里有几处很有意思，其中最显著的便是该函数的行为，它不仅消化了所有与当前生成规则相关的所有语元，还把下一个待解析的语元放进了词法分析器的语元缓冲（该语元与当前的生成规则无关）。这是非常标准的递归下降解析器的行为。下面这个括号运算符的例子更能说明问题：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 213-223

    该函数体现了这个语法解析器的几个特点：

    #.  它展示了\ ``Error``\ 函数的用法。调用该函数时，待解析的语元只能是“\ ``(``\ ”，然而解析完子表达式之后，紧跟着的语元却不一定是“\ ``)``\ ”。比如，要是用户输入的是“\ ``(4 x``\ ”而不是“\ ``(4)``\ ”，语法解析器就应该报错。既然错误时有发生，语法解析器就必须提供一条报告错误的途径：就这个语法解析器而言，应对之道就是返回\ ``NULL``\ 。

    #.  该函数的另一特点在于递归调用了\ ``ParseExpression``\ （很快我们就会看到\ ``ParseExpression``\ 还会反过来调用\ ``ParseParenExpr``\ ）。这种手法简化了递归语法的处理，每一条生成规则的实现都得以变得非常简洁。需要注意的是，我们没有必要为括号构造AST节点。虽然这么做也没错，但括号的作用主要还是对表达式进行分组进而引导语法解析过程。当语法解析器构造完AST之后，括号就没用了。

.. compound::

    下一条生成规则也很简单，它负责处理变量引用和函数调用：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 172-198

    该函数与其它函数的风格别无二致。（调用该函数时当前语元必须是\ ``tok_identifier``\ 。）前文提到的有关递归和错误处理的特点它统统具备。有意思的是这里采用了\ **预读**\ （lookahead）的手段来试探当前标识符的类型，判断它究竟是个独立的变量引用还是个函数调用。只要检查紧跟标识符之后的语元是不是“\ ``(``\ ”，它就能知道到底应该构造\ ``VariableExprAST``\ 节点还是\ ``CallExprAST``\ 节点。

.. compound::

    现在，解析各种表达式的代码都已经完成，不妨再添加一个辅助函数，为它们梳理一个统一的入口。我们将上述表达式称为\ **主**\ 表达式（primary expression），具体原因参见本教程的\ :ref:`后续章节 <user-defined-unary-operators>`\ 。在解析各种主表达式时，我们首先要判定待解析表达式的类别：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 225-236

    看完这个函数的定义，你就能明白为什么先前的各个函数能够放心大胆地对\ ``CurTok``\ 的取值作出假设了。这里预读了下一个语元，预先对待解析表达式的类型作出了判断，然后才调用相应的函数进行解析。

基本表达式全都搞定了，下面开始开始着手解析更为复杂的二元表达式。

解析二元表达式
==============

二元表达式的解析难度要大得多，因为它们往往具有二义性。例如，给定字符串“\ ``x+y*z``\ ”，语法解析器既可以将之解析为“\ ``(x+y)*z``\ ”，也可以将之解析为“\ ``x+(y*z)``\ ”。按照通常的数学定义，我们期望解析成后者，因为“\ ``*``\ ”（乘法）的优先级要高于“\ ``+``\ ”（加法）。

.. compound::

    这个问题的解法很多，其中属\ `运算符优先级解析`__\ 最为优雅和高效。这是一种利用二元运算符的优先级来引导递归调用走向的解析技术。首先，我们需要制定一张优先级表：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 150-164,382-388
        :append:
              ...
            }

    最基本的Kaleidoscope语言仅支持4种二元运算符（对于我们英勇无畏的读者来说，再加几个运算符自然是小菜一碟）。函数\ ``GetTokPrecedence``\ 用于查询当前语元的优先级，如果当前语元不是二元运算符则返回\ ``-1``\ 。这里的\ ``map``\ 简化了新运算符的添加，同时也可以证明我们的算法与具体的运算符无关。当然，要想去掉\ ``map``\ 直接在\ ``GetTokPrecedence``\ 中比较优先级也很简单。（甚至可以直接使用定长数组）。

__ http://en.wikipedia.org/wiki/Operator-precedence_parser

有了上面的函数作为辅助，我们就可以开始解析二元表达式了。运算符优先级解析的基本思想就是通过拆解含有二元运算符的表达式来解决可能的二义性问题。以表达式“\ ``a+b+(c+d)*e*f+g``\ ”为例，在进行运算符优先级解析时，它将被视作一串按二元运算符分隔的主表达式。按照这个思路，解析出来的第一个主表达式应该是“\ ``a``\ ”，紧跟着是若干个有序对，即：\ ``[+, b]``\ 、\ ``[+, (c+d)]``\ 、\ ``[*, e]``\ 、\ ``[*, f]``\ 和\ ``[+, g]``\ 。注意，括号表达式也是主表达式，所以在解析二元表达式时无须特殊照顾\ ``(c+d)``\ 这样的嵌套表达式。

.. compound::

    一开始，每个表达式都由一个主表达式打头阵，身后可能还跟着一串由有序对构成的列表，其中有序对的格式为\ ``[binop, primaryexpr]``\ ：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 271-279

    函数\ ``ParseBinOpRHS``\ 用于解析有序对列表（其中\ ``RHS``\ 是Right Hand Side的缩写，表示“右侧”；与此相对应，\ ``LHS``\ 表示“左侧”——译者注）。它的参数包括一个整数和一个指针，其中整数代表运算符优先级，指针则指向当前已解析出来的那部分表达式。注意，单独一个“\ ``x``\ ”也是合法的表达式：也就是说\ ``binoprhs``\ 有可能为空；碰到这种情况时，函数将直接返回作为参数传入的表达式。在上面的例子中，传入\ ``ParseBinOpRHS``\ 的表达式是“\ ``a``\ ”，当前语元是“\ ``+``\ ”。

.. compound::

    传入\ ``ParseBinOpRHS``\ 的优先级表示的是该函数所能处理的\ **最低运算符优先级**\ 。假设语元流中的下一对是“\ ``[+, x]``\ ”，且传入\ ``ParseBinOpRHS``\ 的优先级是\ ``40``\ ，那么该函数将直接返回（因为“\ ``+``\ ”的优先级是\ ``20``\ ）。搞清楚这一点之后，我们再来看\ ``ParseBinOpRHS``\ 的定义，函数的开头是这样的：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 238-248

    这段代码首先检查当前语元的优先级，如果优先级过低就直接返回。由于无效语元（这里指不是二元运算符的语元——译者注）的优先级都被判作\ ``-1``\ ，因此当语元流中的所有二元运算符都被处理完毕时，该检查自然不会通过。如果检查通过，则可知当前语元一定是二元运算符，应该被纳入当前表达式：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 250-256

    就这样，二元运算符处理完毕（并保存妥当）之后，紧跟其后的主表达式也随之解析完毕。至此，本例中的第一对有序对\ ``[+, b]``\ 就构造完了。

.. compound::

    现在表达式的左侧和\ ``RHS``\ 序列中第一对都已经解析完毕，该考虑表达式的结合次序了。路有两条，要么选择“\ ``(a+b) binop unparsed``\ ”，要么选择“\ ``a + (b binop unparsed)``\ ”。为了搞清楚到底该怎么走，我们先预读出“\ ``binop``\ ”，查出它的优先级，再将之与\ ``BinOp``\ （本例中是“\ ``+``\ ”）的优先级相比较：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 258-261

    ``binop``\ 位于“\ ``RHS``\ ”的右侧，如果\ ``binop``\ 的优先级低于或等于当前运算符的优先级，则可知括号应该加在前面，即按“\ ``(a+b) binop ...``\ ”处理。在本例中，当前运算符是“\ ``+``\ ”，下一个运算符也是“\ ``+``\ ”，二者的优先级相同。既然如此，理应按照“\ ``a+b``\ ”来构造AST节点，然后我们继续解析：

    .. code-block:: cpp

              // ... if body omitted ...
            }

            // Merge LHS/RHS.
            LHS = new BinaryExprAST(BinOp, LHS, RHS);
          }  // loop around to the top of the while loop.
        }

    接着上面的例子，“\ ``a+b+``\ ”的前半段被解析成了“\ ``(a+b)``\ ”，于是“\ ``+``\ ”成为当前语元，进入下一轮迭代。上述代码进而将“\ ``(c+d)``\ ”识别为主表达式，并构造出相应的有序对\ ``[+, (c+d)]``\ 。现在，主表达式右侧的\ ``binop``\ 是“\ ``*``\ ”，由于“\ ``*``\ ”的优先级高于“\ ``+``\ ”，负责检查运算符优先级的\ ``if``\ 判断通过，执行流程得以进入\ ``if``\ 语句的内部。

.. compound::

    现在关键问题来了：\ ``if``\ 语句内的代码怎样才能完整解析出表达式的右半部分呢？尤其是，为了构造出正确的AST，变量\ ``RHS``\ 必须完整表达“\ ``(c+d)*e*f``\ ”。出人意料的是，写成代码之后，这个问题出奇地简单：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 258-269
        :emphasize-lines: 5-6

    看一下主表达式右侧的二元运算符，我们发现它的优先级比当前正在解析的\ ``binop``\ 的优先级要高。由此可知，如果自\ ``binop``\ 以右的若干个连续有序对都含有优先级高于“\ ``+``\ ”的运算符，那么就应该把它们全部解析出来，拼成“\ ``RHS``\ ”后返回。为此，我们将最低优先级设为“\ ``TokPrec+1``\ ”，递归调用函数“\ ``ParseBinOpRHS``\ ”。该调用会完整解析出上述示例中的“\ ``(c+d)*e*f``\ ”，并返回构造出的AST节点，这个节点就是“\ ``+``\ ”表达式右侧的\ ``RHS``\ 。

最后，\ ``while``\ 循环的下一轮迭代将会解析出剩下的“\ ``+g``\ ”并将之纳入AST。仅需区区14行代码，我们就完整而优雅地实现了通用的二元表达式解析算法。上述讲解比较简略，这段代码还是颇有些难度的。我建议你找些棘手的例子多跑跑看，好彻底搞明白这段代码背后的原理。

表达式的解析就此告一段落。现在，我们可以将任意语元流喂入语法解析器并逐步从中构造出表达式，直到解析至不属于表达式的语元为止。接下来，我们来处理函数定义等其他结构。

解析其余结构
============

.. compound::

    下面来解析函数原型。在Kaleidoscope语言中，有两处会用到函数原型：一是“\ ``extern``\ ”函数声明，二是函数定义。相关代码很简单，没太大意思（相对于解析表达式的代码而言）：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 281-303

    在此基础之上，函数定义就很简单了，说白了就是一个函数原型再加一个用作函数体的表达式：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 305-314

    除了用于用户自定义函数的前置声明，“\ ``extern``\ ”语句还可以用来声明“\ ``sin``\ ”、“\ ``cos``\ ”等（C标准库）函数。这些“\ ``extern``\ ”语句不过就是些不带函数体的函数原型罢了：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 326-330

    最后，我们还允许用户随时在顶层输入任意表达式并求值。这一特性是通过一个特殊的匿名零元函数（没有任何参数的函数）实现的，所有顶层表达式都定义在这个函数之内：

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 316-324

现在所有零部件都准备完毕了，只需再编写一小段引导代码就可以跑起来了！

引导代码
========

.. compound::

    引导代码很简单，只需在最外层的循环中按当前语元的类型选定相应的解析函数就可以了。这段实在没什么可介绍的，我就单独把最外层循环贴出来好了。完整代码\ :ref:`参见下方 <chapter-2_full-code>`“Top-Level Parsing”那一段。

    .. literalinclude:: _includes/chapter-2_full.cpp
        :language: cpp
        :lines: 364-376

    这段代码最有意思的地方在于我们忽略了顶层的分号。为什么呢？举个例子，当你在命令行中键入“\ ``4 + 5``\ ”后，语法解析器无法判断你键入的内容是否已经完结。如果下一行键入的是“\ ``def foo...``\ ”，则可知顶层表达式就到\ ``4+5``\ 为止；但你也有可能会接着前面的表达式继续输入“\ ``* 6``\ ”。有了顶层的分号，你就可以输入“\ ``4+5;``\ ”，于是语法解析器就能够辨别表达式在何处结束了。

总结
====

.. compound::

    算上注释我们一共只编写了不到400行代码（去掉注释和空行后只有240行），就完整地实现了包括词法分析器、语法解析器及AST生成器在内的最基本的Kaleidoscope语言。由此编译出的可执行文件用于校验Kaleidoscope代码在语法方面的正确性。请看下面这个例子：

    ::

        $ ./a.out
        ready> def foo(x y) x+foo(y, 4.0);
        Parsed a function definition.
        ready> def foo(x y) x+y y;
        Parsed a function definition.
        Parsed a top-level expr
        ready> def foo(x y) x+y );
        Parsed a function definition.
        Error: unknown token when expecting an expression
        ready> extern sin(a);
        ready> Parsed an extern
        ready> ^D
        $

    可扩展的地方还有很多。通过定义新的AST节点，你可以按各种方式对语言进行扩展。在\ :doc:`下一章 <chapter-3>`\ 中，我们将介绍如何利通过AST生成LLVM中间语言（IR，Intermediate Representation）。

.. _chapter-2_full-code:

完整源码
========

以下是与本章和上一章内容相对应的完整代码。注意这段代码是独立的：无需链接LLVM或其他库即可编译运行。（当然，C和C++标准库除外。）编译方法如下：

.. code-block:: bash

    # Compile
    clang++ -g -O3 toy.cpp
    # Run
    ./a.out

代码如下：

.. literalinclude:: _includes/chapter-2_full.cpp
    :language: cpp

.. vim:ft=rst ts=4 sw=4 et wrap
