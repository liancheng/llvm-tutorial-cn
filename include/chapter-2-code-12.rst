.. code-block:: cpp

    /// binoprhs
    ///   ::= ('+' primary)*
    static ExprAST *ParseBinOpRHS(int ExprPrec, ExprAST *LHS) {
      // If this is a binop, find its precedence.
      while (1) {
        int TokPrec = GetTokPrecedence();
        
        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
          return LHS;
