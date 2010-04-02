.. code-block:: cpp

    /// numberexpr ::= number
    static ExprAST *ParseNumberExpr() {
      ExprAST *Result = new NumberExprAST(NumVal);
      getNextToken(); // consume the number
      return Result;
    }
