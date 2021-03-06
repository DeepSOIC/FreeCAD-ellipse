#include "PreCompiled.h"

#include "ConstraintPy.h"
#include "ConstraintPy.cpp"

#include "ValueSetPy.h"
#include "PyUtils.h"

using namespace FCS;
using DualNumber = Base::DualNumber;


int ConstraintPy::initialization()
{
    getConstraintPtr()->_twin = this;
    return 0;
}
int ConstraintPy::finalization()
{
    return 0;
}

// returns a string which represents the object e.g. when printed in python
std::string ConstraintPy::representation(void) const
{
    return ParaObjectPy::representation();
}

PyObject* ConstraintPy::netError(PyObject* args)
{
    PyObject* vals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &vals))
        return nullptr;

    getConstraintPtr()->throwIfIncomplete();

    HValueSet hvals (vals, false);
    DualNumber ret = getConstraintPtr()->netError(*hvals);
    return ret.getPyObject();

}

PyObject* ConstraintPy::errorVec(PyObject* args)
{
    PyObject* pyvals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &pyvals))
        return nullptr;

    getConstraintPtr()->throwIfIncomplete();

    HValueSet vals(pyvals, false);
    std::vector<DualNumber> errvec(getConstraintPtr()->rank());
    getConstraintPtr()->error(*vals, errvec.data());

    Py::List ret(errvec.size());
    for(size_t i = 0; i < errvec.size(); ++i){
        ret[i] = Py::asObject(errvec[i].getPyObject());
    }
    return Py::new_reference_to(ret);
}

PyObject* ConstraintPy::calculateDatum(PyObject* args)
{
    PyObject* pyvals = Py_None;
    if (!PyArg_ParseTuple(args, "O!", &(ValueSetPy::Type), &pyvals))
        return nullptr;
    std::vector<DualNumber> ret = getConstraintPtr()->calculateDatum(*HValueSet(pyvals, false));
    return Py::new_reference_to(asPyList(ret));
}

Py::Float ConstraintPy::getWeight(void) const
{
    return Py::Float(getConstraintPtr()->weight());
}

void  ConstraintPy::setWeight(Py::Float arg)
{
    getConstraintPtr()->setWeight(arg.as_double());
}

Py::Boolean ConstraintPy::getReversed(void) const
{
    return Py::Boolean(getConstraintPtr()->isReversed());
}

void  ConstraintPy::setReversed(Py::Boolean arg)
{
    getConstraintPtr()->setReversed(arg.as_bool());
}

Py::Float ConstraintPy::getNetError(void) const
{
    try {
        getConstraintPtr()->throwIfIncomplete();
        return Py::Float(getConstraintPtr()->netError());
    } catch (Base::Exception& e) {
        auto pye = e.getPyExceptionType();
        if(!pye)
            pye = Base::BaseExceptionFreeCADError;
        throw Py::Exception(pye, e.what());
    }
}

Py::Long ConstraintPy::getRank(void) const
{
    return Py::Long(getConstraintPtr()->rank());
}

Py::List ConstraintPy::getDatumParameters(void) const
{
    auto params = getConstraintPtr()->datumParameters();
    return asPyList(params);
}

PyObject* ConstraintPy::getCustomAttributes(const char* attr) const
{
    return ParaObjectPy::getCustomAttributes(attr);
}

int ConstraintPy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ParaObjectPy::setCustomAttributes(attr, obj);
}
