.. code-block:: cpp

    /// primary
    ///   ::= identifierexpr
    ///   ::= numberexpr
    ///   ::= parenexpr
    static ExprAST *ParsePrimary() {
      switch (CurTok) {
      default: return Error("unknown token when expecting an expression");
      case tok_identifier: return ParseIdentifierExpr();
      case tok_number:     return ParseNumberExpr();
      case '(':            return ParseParenExpr();
      }
    }
