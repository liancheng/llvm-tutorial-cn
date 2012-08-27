.. _chapter-1:

***************************
第一章 教程介绍与词法分析器
***************************

`本章原文`__

__ http://llvm.org/docs/tutorial/LangImpl1.html

教程介绍
========

欢迎进入本教程——“用LLVM开发新语言”。本教程详细描述了一门简单语言的实现过程，你将会发现这个过程是多么的简单而有趣。本教程将会带您起步，同时协助您构建一个可用以扩展成其他语言框架。教程中的代码可以作为尝试LLVM中其他相关功能的试验田。

本教程的目的在于渐进地揭示我们的语言，详述它的构建过程。期间我们将广泛覆盖有关语言设计和LLVM用法的大量问题。我们会沿途展示和阐述相关的代码，让您不至被大量的细节弄得晕头转向。

得提前说明的是，本教程讲授的是编译器技术和LLVM，而\ **不是**\ 健全的、现代化的软件工程原则。在实际操作中，这意味着我们将走一些捷径以简化阐述。比如代码中会有内存泄漏、到处是全局变量、不会用到\ `visitor`__\ 等良好的模式等等……一切从简。如果您打算深究这些代码以作为未来项目的基础，填补这些缺陷也并不是什么难事。

__ http://en.wikipedia.org/wiki/Visitor_pattern

我对教程内容按章节进行了组织，如果您对某些内容很熟悉或不感兴趣便可直接跳过。教程结构如下：

- :doc:`第一章 <chapter-1>`\ ：\ **Kaleidoscope语言的结构，以及词法解析器的定义**

  说明我们想要做些什么以及希望新语言具备的功能。为了最大限度地令这份教程易于理解和修改，我们选用C++而不使用任何的词法和语法分析器的生成器。当然LLVM可以很好地与这些工具协作，如果愿意您大可选择一种来用。

- :doc:`第二章 <chapter-2>`\ ：\ **实现语法分析器和AST**

  词法分析器到位后，我们便可以开始讨论语法分析技术和基本的AST构建。这份教程中描述了递归下降解析和算符优先级解析。第一章和第二章的内容与LLVM完全无关，我们的代码截至此时甚至都不会链接LLVM。\ :)

- :doc:`第三章 <chapter-3>`\ ：\ **LLVM IR代码生成**

  准备好AST后，我们将展示一下LLVM IR的生成有多么简单。

- :doc:`第四章 <chapter-4>`\ ：\ **添加JIT和优化支持**

  因为很多人对于将LLVM用作JIT感兴趣，所以我们便投身于此并向您展示如何使用3行代码来增加JIT支持。LLVM在其他方面的用途也很广泛，但这恐怕是最简单而“性感”地展示其能力的方式。

- :doc:`第五章 <chapter-5>`\ ：\ **对语言进行扩展：控制流程**

  语言已经就绪并可以运行了，我们再来展示一下如何给它增加控制流程操作（\ ``if``/``then``/``else``\ 和“\ ``for``\ ”循环）。在此我们将有机会讨论一下简单的\ `SSA`__\ 构建与控制流程。

- :doc:`第六章 <chapter-6>`\ ：\ **对语言进行扩展：用户自定义运算符**

  这个章节没什么用途但却很有趣，它讨论的是如何让用户程序自行定义任意的一元和二元运算符（可设置优先级！）。这使得我们可以以库程序的方式来实现该“语言”的很大一部分。

- :doc:`第七章 <chapter-7>`\ ：\ **对语言进行扩展：可变变量**

  本章讲述用赋值运算符添加用户自定义局部变量。有趣之处在于在LLVM中构造SSA形式极其简单：不，实际上LLVM根本就\ **不**\ 要求前端构造SSA形式！

- :doc:`第八章 <chapter-8>`\ ：\ **结论及LLVM相关的其他有用内容**

  本章对扩展语言的潜在方法做了讨论，以此对本系列做了一个了结。同时也包含了一些“特殊主题”的参考信息，如添加垃圾回收支持、异常、调试、“\ `意面式堆栈`__\ ”支持，以及很多其他的窍门和技巧。

__ http://en.wikipedia.org/wiki/Spaghetti_stack
__ http://en.wikipedia.org/wiki/Static_single_assignment_form

到本教程结束时，不算注释和空行，我们总共将编写不到700行代码。借助这些代码，我们为一个相对复杂的语言搭建了一个非常靠谱的编译器，包括手工打造的词法分析器、语法分析器、AST，以及带JIT编译器的代码生成支持。在别的系统只提供了些有那么点意思的“hello world”教程的时候，我想这篇教程的广度足以诠释LLVM的强大，以及当您对语言和编译器设计感兴趣时为何应该考虑使用LLVM。

最后说一句：我们希望您能够自己把玩这个语言并对其进行扩展。拿着代码疯狂地hack去吧，编译器并不总是令人恐怖的怪兽——把玩一门语言是非常有趣的！

基础语言
========

本教程将用一个我们称之为“\ `Kaleidoscope`__\ ”（引申为“美丽、形态，和视图”）\ [#]_\ 的玩具语言来进行说明。Kaleidoscope是一个过程式语言，允许您定义函数、使用条件语句、进行数学运算，等等。在整过教程过程中，我们将逐步扩展Kaleidoscope为其增加\ ``if``/``then``/``else``\ 结构、\ ``for``\ 循环、用户自定义运算符、一个具备简单命令行界面的JIT编译器等等。

__ http://en.wikipedia.org/wiki/Kaleidoscope

为了一切从简，Kaleidoscope唯一支持的数据类型就是64位浮点数（也就是C中的“double”）。如此一来，所有的值都隐含为双精度，同时语言也无须类型申明。这赋予了该语言良好而简单的语法。例如，以下是一个计算\ `斐波那契数`__\ 的简单示例：

::

    # Compute the x'th fibonacci number.
    def fib(x)
      if x < 3 then
        1
      else
        fib(x-1)+fib(x-2)

    # This expression will compute the 40th number.
    fib(40)

__ http://en.wikipedia.org/wiki/Fibonacci_number

我们也允许Kaleidoscope直接调用标志库函数（LLVM JIT使之极其简单）。这意味着您可以在使用一个函数之前先用“extern”关键字来定义它（这对相互递归的函数也很有用）。例如：

::

    extern sin(arg);
    extern cos(arg);
    extern atan2(arg1 arg2);

    atan2(sin(.4), cos(42))

第六章中有一个更有趣的示例：我们编写了一个Kaleidoscope小应用，用以在不同放大尺度上\ `显示Mandelbrot集合`__\ 。

__ http://llvm.org/docs/tutorial/LangImpl6.html#example

让我们来细细品味一下这个语言的实现过程吧！

词法分析器
==========

想要实现一门语言时，第一件事就是要有能力去处理一个文本文件以搞明白它在说什么。传统的做法是使用“\ `词法分析器`__\ ”（也称“扫描器”）将输入切为“标记（token）”。词法分析器返回的每个标记都包含一个标记代码，并可能带有一些元数据（例如一个数字的数值）。首先，我们要定义可能出现的标记：

.. code-block:: cpp

    // The lexer returns tokens [0-255] if it is an unknown character, otherwise one
    // of these for known things.
    enum Token {
      tok_eof = -1,

      // commands
      tok_def = -2, tok_extern = -3,

      // primary
      tok_identifier = -4, tok_number = -5,
    };

    static std::string IdentifierStr;  // Filled in if tok_identifier
    static double NumVal;              // Filled in if tok_number

__ http://en.wikipedia.org/wiki/Lexical_analysis

由我们的词法分析器返回的标记，要么是上述的标记枚举值之一，要么是一个像“+”这样的“未知”字符，这种情况下词法分析器将返回这些字符的ASCII值。若当前的标记是一个标识符，则标识符的名称将被存放于全局变量\ ``IdentifierStr``\ 中。若当前标记是一个数值常量（比如1.0），其值将被存放于\ ``NumVal``\ 中。注意出于简单起见我们使用了全局变量，对实际的语言实现而言这并非最佳选择 :) 。

实际的词法分析器由一个名为\ ``gettok``\ 的函数实现。\ ``gettok``\ 函数被调用时将返回标准输入中的下一个标记。其定义以此起始：

.. code-block:: cpp

    static int gettok() {
      static int LastChar = ' ';

      // Skip any whitespace.
      while (isspace(LastChar))
        LastChar = getchar();

``gettok``\ 调用C的\ ``getchar()``\ 函数从标准输入中一次一个地读入字符。它吞取和识别字符，同时将读取到的最后一个字符存在\ ``LastChar``\ 中留待处理。首先要做的是忽略掉标记之间的空白符。这由上述的循环完成的。

接下来\ ``gettok``\ 得识别出标识符和特定的关键字，比如“\ ``def``\ ”。Kaleidoscope用下面的简单循环来达到目的：

.. code-block:: cpp

    if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
      IdentifierStr = LastChar;
      while (isalnum((LastChar = getchar())))
        IdentifierStr += LastChar;

      if (IdentifierStr == "def") return tok_def;
      if (IdentifierStr == "extern") return tok_extern;
      return tok_identifier;
    }

注意，这段代码一旦分析出一个标识符，就立即将至存入全局变量\ ``IdentifierStr``\ 中。同时，由于语言中的关键字也在同一个循环中识别，我们在此处一并处理。对数值的识别也类似：

.. code-block:: cpp

    if (isdigit(LastChar) || LastChar == '.') {   // Number: [0-9.]+
      std::string NumStr;
      do {
        NumStr += LastChar;
        LastChar = getchar();
      } while (isdigit(LastChar) || LastChar == '.');

      NumVal = strtod(NumStr.c_str(), 0);
      return tok_number;
    }

这些处理输入的代码都很直截了当。当从输入中读到表征数值的字符串时，我们使用C的\ ``strtod``\ 函数将之转换为数值并存入\ ``NumVal``\ 。注意这里并没有做充分的错误检测：这段代码会错误地识别出“1.23.45.67”并将之当作“1.23”来处理。乐意的话请随意修改 :) 。下面我们来处理注释：

.. code-block:: cpp

    if (LastChar == '#') {
      // Comment until end of line.
      do LastChar = getchar();
      while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

      if (LastChar != EOF)
        return gettok();
    }

我们直接跳过注释行并返回下一个标记。最后，如果输入与上述情况都不相符，则他要么是一个诸如“+”的运算符字符，要么就是已经抵达文件末尾。这种情况有下面的代码来处理：

.. code-block:: cpp

      // Check for end of file.  Don't eat the EOF.
      if (LastChar == EOF)
        return tok_eof;

      // Otherwise, just return the character as its ascii value.
      int ThisChar = LastChar;
      LastChar = getchar();
      return ThisChar;
    }

到此为止，我们已经拥有了Kaleidoscope语言的一个完整的词法分析器（词法分析器的\ :ref:`完整源码 <chapter-2-code>`\ 参见本教程的下一章）。接下来我们将\ :doc:`构建一个简单的语法分析器并借助它来构建抽象语法树 <chapter-2>`\ 。届时，我们还会包含一段驱动代码，这样您就能一并使用词法分析器和语法分析器了。

.. rubric:: 脚注

.. [#] Kaleidoscope即“万花筒”。

.. vim:ft=rst ts=4 sw=4 et wrap
