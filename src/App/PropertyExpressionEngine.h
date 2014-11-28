#ifndef EXPRESSIONENGINE_H
#define EXPRESSIONENGINE_H

#include <map>
#include <App/Property.h>

namespace Base {
class Writer;
class XMLReader;
}

namespace App {

class Expression;
class DocumentObject;
class DocumentObjectExecReturn;

class PropertyExpressionEngine : public App::Property
{
    TYPESYSTEM_HEADER();
public:
    PropertyExpressionEngine(App::DocumentObject *_docObj = 0);
    ~PropertyExpressionEngine();

    unsigned int getMemSize (void) const;

    void setValue() { } // Dummy

    Property *Copy(void) const;

    void Paste(const Property &from);

    void Save (Base::Writer & writer) const;

    void Restore(Base::XMLReader &reader);

    void setValue(const App::Path &path, const Expression * expr);

    const Expression * getValue(const Path & path) const {
        std::map<const Path, const Expression*>::const_iterator i = expressions.find(path);
        if (i != expressions.end())
            return i->second;
        else
            return 0;
    }

    DocumentObjectExecReturn * execute();

private:
    bool running;
    App::DocumentObject * docObj;
    std::map<const Path, const Expression*> expressions;
    std::vector<Path> evaluationOrder;
};

}

#endif // EXPRESSIONENGINE_H
