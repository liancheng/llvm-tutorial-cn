ExprAST *X = new VariableExprAST("x");
ExprAST *Y = new VariableExprAST("y");
ExprAST *Result = new BinaryExprAST('+', X, Y);
