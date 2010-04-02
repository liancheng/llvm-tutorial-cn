.. code-block:: cpp

    /// identifierexpr
    ///   ::= identifier
    ///   ::= identifier '(' expression* ')'
    static ExprAST *ParseIdentifierExpr() {
      std::string IdName = IdentifierStr;
      
      getNextToken();  // eat identifier.
      
      if (CurTok != '(') // Simple variable ref.
        return new VariableExprAST(IdName);
      
      // Call.
      getNextToken();  // eat (
      std::vector<ExprAST*> Args;
      if (CurTok != ')') {
        while (1) {
          ExprAST *Arg = ParseExpression();
          if (!Arg) return 0;
          Args.push_back(Arg);

          if (CurTok == ')') break;

          if (CurTok != ',')
            return Error("Expected ')' or ',' in argument list");
          getNextToken();
        }
      }

      // Eat the ')'.
      getNextToken();
      
      return new CallExprAST(IdName, Args);
    }
