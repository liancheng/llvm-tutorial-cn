.. _chapter-1:

***************************
第一章 教程简介与词法分析器
***************************

:原文: `Tutorial Introduction and the Lexer <http://llvm.org/docs/tutorial/LangImpl1.html>`_

教程介绍
========

欢迎走进“用LLVM开发新语言”教程。本教程详细介绍了一门简单语言的实现过程；你将会看到，这个过程既轻松又有趣。本教程将伴你一起搭建一套框架，它可以成为你今后实现其他语言的基础。教程中的代码就是供你把玩LLVM的各种功能的试验田。

本教程的目标是循序渐进地描绘我们的语言，并详述它的构建过程。在这一过程中，我们将针对语言设计和LLVM的用法等很多问题进行讨论。与此同时，为了让你不至于被大量的细节弄得晕头转向，我们会给出相关代码并进行讲解。

需要提前说明的是，这一教程讲授的是编译器技术和LLVM，而\ **不是**\ 四平八稳的现代化软件工程准则。换言之，为了方便讲解，我们会采用一些不太正规的手法。比如，代码中存在内存泄漏、遍地是全局变量、无视\ `visitor`__\ 之类的成熟设计模式等等……一切从简。如果你有意深入研究，并把这些代码用作今后项目的基础，这些问题倒也不难修。

__ http://en.wikipedia.org/wiki/Visitor_pattern

我试着编排了本教程的各个章节，碰到熟悉的或是不感兴趣的章节，你大可直接跳过。本教程结构如下：

- :doc:`第一章 <chapter-1>`\ ：\ **Kaleidoscope语言及其词法解析器简介**\ ——说明我们的目的，以及新语言应该具备些什么基本功能。为了尽量让这份教程易于理解和入手，我们决定不采用任何词法分析器和语法分析器的生成器（例如lex/yacc或flex/bison——译者注），而是直接用C++实现所有功能。当然，配合这些工具使用LLVM也完全没问题，如果乐意，你大可一试。

- :doc:`第二章 <chapter-2>`\ ：\ **实现语法分析器和AST**\ ——完成了词法分析器，接下来就该讨论语法解析技术并开始构造基本的AST（Abstract Syntax Tree，抽象语法树——译者注）了。这份教程共介绍了递归下降解析和运算符优先级解析两种解析技术。\ :doc:`第一章 <chapter-1>`\ 和\ :doc:`第二章 <chapter-2>`\ 的内容与LLVM完全无关，至此我们的代码甚至都不需要链接LLVM库。 :)

- :doc:`第三章 <chapter-3>`\ ：\ **LLVM IR代码生成**\ ——搞定AST之后，我们将炫耀一番，让你看看LLVM IR的生成过程是多么的简单。

- :doc:`第四章 <chapter-4>`\ ：\ **添加JIT和优化支持**\ ——鉴于很多人对如何把LLVM用作JIT感兴趣，我们打算在此深入一下，让你看看用于添加JIT支持的那三行代码。尽管LLVM在其他方面的用途也很广泛，但要说炫技，还是这三行代码最简单也最“惊艳”。

- :doc:`第五章 <chapter-5>`\ ：\ **语言扩展：流程控制**\ ——我们的语言已经准备就绪可以运行了，现在来看看如何给它添加流程控制能力（\ ``if``/``then``/``else``\ 和“\ ``for``\ ”循环）。在此我们还将借机介绍一下简单的\ `SSA`__\ 构造和流程控制。

- :doc:`第六章 <chapter-6>`\ ：\ **语言扩展：用户自定义运算符**\ ——这一章虽然无聊但却很有意思，它讨论了如何对语言进行扩展，从而让用户能够在程序中自行定义任意的一元和二元运算符（还可以设置优先级！）。这一机制使得我们得以将该“语言”的很大一部分都实现成库函数。

- :doc:`第七章 <chapter-7>`\ ：\ **语言扩展：可变变量**\ ——本章介绍的是如何添加用户自定义局部变量和赋值运算符。最有趣儿的地方在于，在LLVM中构造SSA form简单至极：不，实际上LLVM压根儿就\ **不**\ 要求你在前端（指编译器/解释器前端——译者注）构造SSA form！

- :doc:`第八章 <chapter-8>`\ ：\ **结论以及其他和LLVM相关的内容**\ ——本章对整个系列教程进行了总结，不仅讨论了多种潜在的语言扩展方向，同时还给出了一系列“专题”的参考资料，例如垃圾回收支持、异常、调试、“\ `意面式栈`__\ ”支持，此外还有很多其他窍门和技巧。

__ http://en.wikipedia.org/wiki/Static_single_assignment_form
__ http://en.wikipedia.org/wiki/Spaghetti_stack

到本教程末尾为止，不算注释和空行，我们总共只需编写不到700行代码。面对这样一门相对复杂的语言，我们只用了这么点儿代码就实现了一个像样的编译器，包括手工打造的词法分析器、语法分析器、AST，还实现了带JIT编译器的代码生成。我想，相对于那些仅能给出“hello world”级别的教程的系统，这篇教程的广度足以诠释LLVM的强大；如果你对语言和编译器设计感兴趣，它应该能够说服你认真考虑考虑LLVM。

最后说一句：我们希望你能够对这一语言进行扩展，好好玩味一番。拿着代码疯狂地hack去吧，编译器并非令人恐惧的怪兽——把玩程序语言，奇乐无穷！

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
