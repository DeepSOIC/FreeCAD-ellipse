#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include "Mod/Part/App/Attacher.h"
#include <Base/PlacementPy.h>
#include "TopoShapePy.h"

#include "OCCError.h"

// inclusion of the generated files (generated out of AttachableObjectPy.xml)
#include "AttachEnginePy.h"
#include "AttachEnginePy.cpp"

using namespace Attacher;

// returns a string which represents the object e.g. when printed in python
std::string AttachEnginePy::representation(void) const
{
    return std::string("<Attacher::AttachEngine>");
}

Py::String AttachEnginePy::getAttacherType(void) const
{
    return  Py::String(std::string(this->getAttachEnginePtr()->getTypeId().getName()));
}

/**
  * @brief macro ATTACHERPY_STDCATCH_ATTR: catch for exceptions in attribute
  * code (re-throws the exception converted to Py::Exception). It is a helper
  * to avoid repeating the same error handling code over and over again.
  */
#define ATTACHERPY_STDCATCH_ATTR \
    catch (Standard_Failure) {\
        Handle_Standard_Failure e = Standard_Failure::Caught();\
        throw Py::Exception(Part::PartExceptionOCCError, e->GetMessageString());\
    } catch (Base::Exception &e) {\
        throw Py::Exception(Base::BaseExceptionFreeCADError, e.what());\
    }

Py::String AttachEnginePy::getMode(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::String(attacher.getModeName(attacher.mapMode));
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setMode(Py::String arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        std::string modeName = (std::string)arg;
        attacher.mapMode = attacher.getModeByName(modeName);
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Object AttachEnginePy::getReferences(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::Object(attacher.references.getPyObject(),true);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setReferences(Py::Object arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        attacher.references.setPyObject(arg.ptr());
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::Object AttachEnginePy::getSuperPlacement(void) const
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        return Py::Object(new Base::PlacementPy(new Base::Placement(attacher.superPlacement)),true);
    } ATTACHERPY_STDCATCH_ATTR;
}

void AttachEnginePy::setSuperPlacement(Py::Object arg)
{
    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        if (PyObject_TypeCheck(arg.ptr(), &(Base::PlacementPy::Type))) {
            const Base::PlacementPy* plmPy = static_cast<const Base::PlacementPy*>(arg.ptr());
            attacher.superPlacement = *(plmPy->getPlacementPtr());
        } else {
            std::string error = std::string("type must be 'Placement', not ");
            error += arg.type().as_string();
            throw Py::TypeError(error);
        }
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::List AttachEnginePy::getCompleteModeList(void) const
{
    try {
        Py::List ret;
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        for(int imode = 0   ;   imode < mmDummy_NumberOfModes   ;   imode++){
            ret.append(Py::String(attacher.getModeName(eMapMode(imode))));
        }
        return ret;
    } ATTACHERPY_STDCATCH_ATTR;
}

Py::List AttachEnginePy::getImplementedModes(void) const
{
    try {
        Py::List ret;
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        for(int imode = 0   ;   imode < mmDummy_NumberOfModes   ;   imode++){
            if(attacher.modeRefTypes[imode].size() > 0){
                ret.append(Py::String(attacher.getModeName(eMapMode(imode))));
            }
        }
        return ret;
    } ATTACHERPY_STDCATCH_ATTR;
}

/**
  * @brief macro ATTACHERPY_STDCATCH_METH: catch for exceptions in method code
  * (returns NULL if an exception is caught). It is a helper to avoid repeating
  * the same error handling code over and over again.
  */
#define ATTACHERPY_STDCATCH_METH \
    catch (Standard_Failure) {\
        Handle_Standard_Failure e = Standard_Failure::Caught();\
        PyErr_SetString(Part::PartExceptionOCCError, e->GetMessageString());\
        return NULL;\
    } catch (Base::Exception &e) {\
        PyErr_SetString(Base::BaseExceptionFreeCADError, e.what());\
        return NULL;\
    }


PyObject* AttachEnginePy::getModeInfo(PyObject* args)
{
    char* modeName;
    if (!PyArg_ParseTuple(args, "s", &modeName))
        return 0;

    try {
        AttachEngine &attacher = *(this->getAttachEnginePtr());
        eMapMode mmode = attacher.getModeByName(modeName);
        Py::List pyListOfCombinations;
        Py::List pyCombination;
        refTypeStringList &listOfCombinations = attacher.modeRefTypes.at(mmode);
        for(const refTypeString &combination: listOfCombinations){
            pyCombination = Py::List(combination.size());
            for(int iref = 0   ;   iref < combination.size()   ;   iref++){
                pyCombination[iref] = Py::String(AttachEngine::getRefTypeName(combination[iref]));
            }
            pyListOfCombinations.append(pyCombination);
        }
        Py::Dict ret;
        ret["ReferenceCombinations"] = pyListOfCombinations;
        ret["ModeIndex"] = Py::Int(mmode);
        return Py::new_reference_to(ret);
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getShapeType(PyObject* args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Part::TopoShapePy::Type), &pcObj))
        return NULL;

    try{
        TopoDS_Shape shape = static_cast<Part::TopoShapePy*>(pcObj)->getTopoShapePtr()->_Shape;
        eRefType rt = AttachEngine::getShapeType(shape);
        return Py::new_reference_to(Py::String(AttachEngine::getRefTypeName(rt)));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::isShapeOfType(PyObject* args)
{
    char* type_shape_str;
    char* type_need_str;
    if (!PyArg_ParseTuple(args, "ss", &type_shape_str, &type_need_str))
        return 0;
    try {
        eRefType type_shape = AttachEngine::getRefTypeByName(std::string(type_shape_str));
        eRefType type_need = AttachEngine::getRefTypeByName(std::string(type_need_str));
        bool result = AttachEngine::isShapeOfType(type_shape, type_need) > -1;
        return Py::new_reference_to(Py::Boolean(result));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::downgradeType(PyObject* args)
{
    char* type_shape_str;
    if (!PyArg_ParseTuple(args, "s", &type_shape_str))
        return 0;
    try {
        eRefType type_shape = AttachEngine::getRefTypeByName(std::string(type_shape_str));
        eRefType result = AttachEngine::downgradeType(type_shape);
        return Py::new_reference_to(Py::String(AttachEngine::getRefTypeName(result)));
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getTypeRank(PyObject* args)
{
    char* type_shape_str;
    if (!PyArg_ParseTuple(args, "s", &type_shape_str))
        return 0;
    try {
        eRefType type_shape = AttachEngine::getRefTypeByName(std::string(type_shape_str));
        int result = AttachEngine::getTypeRank(type_shape);
        return Py::new_reference_to(Py::Int(result));
    } ATTACHERPY_STDCATCH_METH;

}

PyObject* AttachEnginePy::copy(PyObject* args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    return new AttachEnginePy(this->getAttachEnginePtr()->copy());
}

PyObject* AttachEnginePy::calculateAttachedPlacement(PyObject* args)
{
    PyObject *pcObj;
    if (!PyArg_ParseTuple(args, "O!", &(Base::PlacementPy::Type), &pcObj))
        return NULL;
    try{
        const Base::Placement& plm = *(static_cast<const Base::PlacementPy*>(pcObj)->getPlacementPtr());
        Base::Placement result;
        try{
            result = this->getAttachEnginePtr()->calculateAttachedPlacement(plm);
        } catch (ExceptionCancel) {
            Py_IncRef(Py_None);
            return Py_None;
        }
        return new Base::PlacementPy(new Base::Placement(result));
    } ATTACHERPY_STDCATCH_METH;

    return NULL;
}


PyObject* AttachEnginePy::getCustomAttributes(const char*) const
{
    return 0;
}

int AttachEnginePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

