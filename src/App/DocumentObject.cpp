/***************************************************************************
 *   Copyright (c) J�rgen Riegel          (juergen.riegel@web.de)          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <Base/Writer.h>
#include <Base/Interpreter.h>

#include "Application.h"
#include "Document.h"
#include "DocumentObject.h"
#include "DocumentObjectPy.h"
#include "DocumentObjectGroup.h"
#include "PropertyLinks.h"
#include "PropertyUnits.h"
#include "PropertyExpressionEngine.h"
#include "Expression.h"


using namespace App;


PROPERTY_SOURCE(App::DocumentObject, App::PropertyContainer)

DocumentObjectExecReturn *DocumentObject::StdReturn = 0;

//===========================================================================
// DocumentObject
//===========================================================================

DocumentObject::DocumentObject(void)
    : _pDoc(0),pcNameInDocument(0),ExpressionEngine(this)
{
    // define Label of type 'Output' to avoid being marked as touched after relabeling
    ADD_PROPERTY_TYPE(Label,("Unnamed"),"Base",Prop_Output,"User name of the object (UTF8)");
    ADD_PROPERTY_TYPE(ExpressionEngine,(),"Base",Prop_Hidden,"Property expressions");
}

DocumentObject::~DocumentObject(void)
{
    if (!PythonObject.is(Py::_None())){
        // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed 
        // Python object or not. In the constructor we forced the wrapper to own the object so we need
        // not to dec'ref the Python object any more.
        // But we must still invalidate the Python object because it need not to be
        // destructed right now because the interpreter can own several references to it.
        Base::PyObjectBase* obj = (Base::PyObjectBase*)PythonObject.ptr();
        // Call before decrementing the reference counter, otherwise a heap error can occur
        obj->setInvalid();
    }
}

namespace App {
class ObjectExecution
{
public:
    ObjectExecution(DocumentObject* o) : obj(o)
    { obj->StatusBits.set(3); }
    ~ObjectExecution()
    { obj->StatusBits.reset(3); }
private:
    DocumentObject* obj;
};
}

App::DocumentObjectExecReturn *DocumentObject::recompute(void)
{
    // set/unset the execution bit
    ObjectExecution exe(this);

    ExpressionEngine.execute();
    return this->execute();
}

DocumentObjectExecReturn *DocumentObject::execute(void)
{
    return StdReturn;
}

short DocumentObject::mustExecute(void) const
{
    return (isTouched() ? 1 : 0);
}

const char* DocumentObject::getStatusString(void) const
{
    if (isError()) {
        const char* text = getDocument()->getErrorDescription(this);
        return text ? text : "Error";
    }
    else if (isTouched())
        return "Touched";
    else
        return "Valid";
}

const char *DocumentObject::getNameInDocument(void) const
{
    // Note: It can happen that we query the internal name of an object even if it is not
    // part of a document (anymore). This is the case e.g. if we have a reference in Python 
    // to an object that has been removed from the document. In this case we should rather
    // return 0.
    //assert(pcNameInDocument);
    if (!pcNameInDocument) return 0;
    return pcNameInDocument->c_str();
}

std::vector<DocumentObject*> DocumentObject::getOutList(void) const
{
    std::vector<Property*> List;
    std::vector<DocumentObject*> ret;
    getPropertyList(List);
    for (std::vector<Property*>::const_iterator It = List.begin();It != List.end(); ++It) {
        if ((*It)->isDerivedFrom(PropertyLinkList::getClassTypeId())) {
            const std::vector<DocumentObject*> &OutList = static_cast<PropertyLinkList*>(*It)->getValues();
            for (std::vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    ret.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSubList::getClassTypeId())) {
            const std::vector<DocumentObject*> &OutList = static_cast<PropertyLinkSubList*>(*It)->getValues();
            for (std::vector<DocumentObject*>::const_iterator It2 = OutList.begin();It2 != OutList.end(); ++It2) {
                if (*It2)
                    ret.push_back(*It2);
            }
        }
        else if ((*It)->isDerivedFrom(PropertyLink::getClassTypeId())) {
            if (static_cast<PropertyLink*>(*It)->getValue())
                ret.push_back(static_cast<PropertyLink*>(*It)->getValue());
        }
        else if ((*It)->isDerivedFrom(PropertyLinkSub::getClassTypeId())) {
            if (static_cast<PropertyLinkSub*>(*It)->getValue())
                ret.push_back(static_cast<PropertyLinkSub*>(*It)->getValue());
        }

        /* Get property dependencies */
        const std::vector<PropertyDependencyLink> & links = (*It)->getDependencies().getLinks();
        std::vector<PropertyDependencyLink>::const_iterator It3 = links.begin();
        while (It3 != links.end()) {
            PropertyContainer * container = (*It3).getProperty()->getContainer();

            if (container->isDerivedFrom(DocumentObject::getClassTypeId()))
                if (container != this)
                    ret.push_back(static_cast<DocumentObject*>(container));

            ++It3;
        }

    }
    return ret;
}

std::vector<App::DocumentObject*> DocumentObject::getInList(void) const
{
    if (_pDoc)
        return _pDoc->getInList(this);
    else
        return std::vector<App::DocumentObject*>();
}

DocumentObjectGroup* DocumentObject::getGroup() const
{
    return DocumentObjectGroup::getGroupOfObject(this);
}

void DocumentObject::onLostLinkToObject(DocumentObject*)
{

}

App::Document *DocumentObject::getDocument(void) const
{
    return _pDoc;
}

void DocumentObject::setDocument(App::Document* doc)
{
    _pDoc=doc;
    onSettingDocument();
}

void DocumentObject::onBeforeChange(const Property* prop)
{
    if (_pDoc)
        _pDoc->onBeforeChangeProperty(this,prop);
}

/// get called by the container when a Property was changed
void DocumentObject::onChanged(const Property* prop)
{
    if (_pDoc)
        _pDoc->onChangedProperty(this,prop);
    if (prop->getType() & Prop_Output)
        return;
    // set object touched
    StatusBits.set(0);
}

PyObject *DocumentObject::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

std::vector<PyObject *> DocumentObject::getPySubObjects(const std::vector<std::string>&) const
{
    // default implementation returns nothing
    return std::vector<PyObject *>();
}

void DocumentObject::touch(void)
{
    StatusBits.set(0);
}

bool DocumentObject::isTouched() const
{
     /* Check property dependencies */
     std::vector<Property*> List;

     getPropertyList(List);
     for (std::vector<Property*>::const_iterator It = List.begin();It != List.end(); ++It) {

         const std::vector<PropertyDependencyLink> & links = (*It)->getDependencies().getLinks();
         std::vector<PropertyDependencyLink>::const_iterator It2 = links.begin();

         while (It2 != links.end()) {
             if ((*It2).getProperty()->isTouched())
                 return true;

             ++It2;
         }
     }

     if (StatusBits.test(0))
         return true;

     return false;
}

void DocumentObject::Save (Base::Writer &writer) const
{
    writer.ObjectName = this->getNameInDocument();
    App::PropertyContainer::Save(writer);
}

void DocumentObject::setExpression(const Path path, const Expression *expr)
{
    ExpressionEngine.setValue(path, expr);
}

const Expression * DocumentObject::getExpression(const Path & path) const
{
    return ExpressionEngine.getValue(path);
}

Property *DocumentObject::getPropertyByPath(const Path &path)
{
    return getPropertyByName(path.getPropertyName().c_str());
}

const Property *DocumentObject::getPropertyByPath(const Path &path) const
{
    return getPropertyByName(path.getPropertyName().c_str());
}

void DocumentObject::setValue(const Path & path, const Expression *result)
{
    std::stringstream cmd;
    std::string docObjectStr = path.getDocumentObjectName();

    if (docObjectStr.size() == 0)
        docObjectStr = getNameInDocument();

    std::string pathStr = getPropertyByPath(path)->getName() + path.getSubPathStr();

    cmd << "FreeCAD.getDocument(\"" << App::GetApplication().getDocumentName(getDocument()) << "\")" <<
           ".getObject(\"" << docObjectStr << "\")." << pathStr <<
           " = " << result->toString();

    Base::Interpreter().runString(cmd.str().c_str());
}

Expression *DocumentObject::getValue(const Path &path)
{
    const Property * prop = getPropertyByPath(path);

    assert(prop != 0);

    if (prop->isDerivedFrom(PropertyQuantity::getClassTypeId())) {
        const PropertyQuantity * value = static_cast<const PropertyQuantity*>(prop);
        return new NumberExpression(this, value->getValue(), value->getUnit());
    }
    else if (prop->isDerivedFrom(PropertyFloat::getClassTypeId())) {
        const PropertyFloat * value = static_cast<const PropertyFloat*>(prop);
        return new NumberExpression(this, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyInteger::getClassTypeId())) {
        const PropertyInteger * value = static_cast<const PropertyInteger*>(prop);
        return new NumberExpression(this, value->getValue());
    }
    else if (prop->isDerivedFrom(PropertyString::getClassTypeId())) {
        const PropertyString * value = static_cast<const PropertyString*>(prop);
        return new StringExpression(this, value->getValue());
    }

    throw Base::Exception("Property is of invalid type (not float).");
}
