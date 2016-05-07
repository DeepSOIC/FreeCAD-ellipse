#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include "Mod/Part/App/Attacher.h"

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
                pyCombination[iref] = Py::Int(combination[iref]);//TODO: change to string
            }
            pyListOfCombinations.append(pyCombination);
        }
        Py::Dict ret;
        ret["ReferenceCombinations"] = pyListOfCombinations;
        return Py::new_reference_to(ret);
    } ATTACHERPY_STDCATCH_METH;
}

PyObject* AttachEnginePy::getCustomAttributes(const char*) const
{
    return 0;
}

int AttachEnginePy::setCustomAttributes(const char*,PyObject*)
{
    return 0;
}

