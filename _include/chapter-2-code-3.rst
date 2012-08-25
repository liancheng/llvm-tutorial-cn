.. code-block:: cpp

    /// PrototypeAST - This class represents the "prototype" for a function,
    /// which captures its name, and its argument names (thus implicitly the number
    /// of arguments the function takes).
    class PrototypeAST {
      std::string Name;
      std::vector<std::string> Args;
    public:
      PrototypeAST(const std::string &name, const std::vector<std::string> &args)
        : Name(name), Args(args) {}
    };

    /// FunctionAST - This class represents a function definition itself.
    class FunctionAST {
      PrototypeAST *Proto;
      ExprAST *Body;
    public:
      FunctionAST(PrototypeAST *proto, ExprAST *body)
        : Proto(proto), Body(body) {}
    };
