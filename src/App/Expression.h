#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <string>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Base/Unit.h>
#include <App/Property.h>
#include <set>

namespace App  {

class Document;
class Expression;

class AppExport ExpressionVisitor {
public:
    virtual ~ExpressionVisitor() {}
    virtual void visit(Expression * e) = 0;
};

/**
  * Base class for expressions.
  *
  */

class AppExport Expression {
public:

    Expression(const App::DocumentObject * _owner);

    virtual ~Expression();

    virtual bool isTouched() const { return false; }

    virtual Expression * eval() const = 0;

    virtual std::string toString() const = 0;

    static Expression * parse(const App::DocumentObject * owner, const std::string& buffer);

    virtual Expression * copy() const = 0;

    virtual int priority() const { return 0; }

    virtual void getDeps(std::set<const App::Property*> &props) const { }

    virtual void getDeps(std::set<Path> &props) const { }
    virtual Expression * simplify() const = 0;

    virtual void visit(ExpressionVisitor & v) { v.visit(this); }

    class Exception : public Base::Exception {
    public:
        Exception(const char *sMessage) : Base::Exception(sMessage) { }
    };

    class CircularException : public Exception {
    public:
        CircularException() : Exception("Circular expression.") { }
    };

    class UnitException : public Exception {
    public:
        UnitException(const char *sMessage) : Exception(sMessage) { }
    };

    const App::DocumentObject *  getOwner() const { return owner; }

protected:
    const App::DocumentObject * owner; /**< The document object used to access unqualified variables (i.e local scope) */
};

/**
  * Part of an expressions that contains a unit.
  *
  */

class AppExport UnitExpression : public Expression {
public:
    UnitExpression(const App::DocumentObject *_owner, const Base::Unit & _unit = Base::Unit(), std::string _unitstr = "", double _scaler = 1.0);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    void setUnit(const Base::Unit & _unit, std::string _unitstr, double _scaler);

    const Base::Unit & getUnit() const { return unit; }

    const std::string getUnitString() const { return unitstr; }

    double getScaler() const { return scaler; }

protected:
    Base::Unit unit;     /**< Unit for this value */
    std::string unitstr; /**< The unit string from the original parsed string */
    double scaler;       /**< The scale factor used to convert to the internal unit */
};

/**
  * Class implementing a number with an optional unit
  */

class AppExport NumberExpression : public UnitExpression {
public:
    NumberExpression(const App::DocumentObject *_owner, double _value, const Base::Unit & _units = Base::Unit(), std::string _unitstr = "", double _scaler = 1.0);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    double getValue() const { return value * getScaler(); }

    void negate();

protected:
    double value; /**< Value */
};

/**
  * Class implementing an infix expression.
  *
  */

class AppExport OperatorExpression : public UnitExpression {
public:
    enum Operator {
        ADD,
        SUB,
        MUL,
        DIV,
        POW,
        UNIT
    };
    OperatorExpression(const App::DocumentObject *_owner, Expression * _left, Operator _op, Expression * _right);

    virtual ~OperatorExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const;

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<Path> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:
    Operator op;        /**< Operator working on left and right */
    Expression * left;  /**< Left operand */
    Expression * right; /**< Right operand */
};

/**
  * Class implementing various functions, e.g sin, cos, etc.
  *
  */

class AppExport FunctionExpression : public UnitExpression {
public:
    enum Function {
        ACOS,
        ASIN,
        ATAN,
        ABS,
        EXP,
        LOG,
        LOG10,
        SIN,
        SINH,
        TAN,
        TANH,
        SQRT,
        COS,
        COSH,
        ATAN2,
        MOD,
        POW,
    };

    FunctionExpression(const App::DocumentObject *_owner, Function _f, Expression * _arg1 , Expression * _arg2 = 0);

    virtual ~FunctionExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const;

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<Path> &props) const;

    virtual void visit(ExpressionVisitor & v);

protected:
    Function f;        /**< Function to execute */
    Expression * arg1; /**< First argument to function */
    Expression * arg2; /**< Optional second argument to function */
};

/**
  * Class implementing a reference to a property. If the name is unqualified,
  * the owner of the expression is searched. If it is qualified, the document
  * that contains the owning document object is searched for other document
  * objects to search. Both labels and internal document names are searched.
  *
  */

class AppExport VariableExpression : public UnitExpression {
public:
    VariableExpression(const App::DocumentObject *_owner, Path _var);

    ~VariableExpression();

    virtual bool isTouched() const;

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return var.toString(); }

    virtual Expression * copy() const;

    virtual int priority() const { return 10; }

    virtual void getDeps(std::set<const App::Property*> &props) const;

    virtual void getDeps(std::set<Path> &props) const;

    std::string name() const { return var.getPropertyName(); }

    Path getPath() const { return var; }

    void setName(const std::string & name) { assert(0); }

protected:

    const App::Property *getProperty() const;

    Path var; /**< Variable name  */
};

/**
  * Class implementing a string. Used to signal either a genuine string or
  * a failed evaluation of an expression.
  */

class AppExport StringExpression : public Expression {
    public:
    StringExpression(const App::DocumentObject *_owner, const std::string & _text);

    virtual Expression * eval() const;

    virtual Expression * simplify() const;

    virtual std::string toString() const { return text; }

    virtual Expression * copy() const;

protected:
    std::string text; /**< Text string */
};

namespace ExpressionParser {
AppExport Expression * parse(const App::DocumentObject *owner, const char *buffer);
AppExport UnitExpression * parseUnit(const App::DocumentObject *owner, const char *buffer);
AppExport App::Path parsePath(const App::DocumentObject *owner, const char* buffer);
}

}
#endif // EXPRESSION_H
