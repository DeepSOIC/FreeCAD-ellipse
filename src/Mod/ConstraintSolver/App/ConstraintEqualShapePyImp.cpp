#include "PreCompiled.h"

#include "ConstraintEqualShapePy.h"
#include "ConstraintEqualShapePy.cpp"

#include "ConstraintPy.h"
#include "ValueSetPy.h"
#include "PyUtils.h"

PyObject *ConstraintEqualShapePy::PyMake(struct _typeobject *, PyObject* args, PyObject* kwd)  // Python wrapper
{
    return pyTryCatch([&]()->Py::Object{

        if (!PyArg_ParseTuple(args, "")){
            PyErr_SetString(PyExc_TypeError, "Only keyword arguments are supported");
            throw Py::Exception();
        }
        HConstraintEqualShape p = (new ConstraintEqualShape)->getHandle<ConstraintEqualShape>();
        if (kwd && kwd != Py_None)
            p->initFromDict(Py::Dict(kwd));
        return p.getHandledObject();
    });
}

// constructor method
int ConstraintEqualShapePy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

std::string ConstraintEqualShapePy::representation(void) const
{
    return getConstraintEqualShapePtr()->repr();
}


Py::Boolean ConstraintEqualShapePy::getEqualTrim(void) const
{
    return Py::Boolean(getConstraintEqualShapePtr()->equalTrim);
}

void  ConstraintEqualShapePy::setEqualTrim(Py::Boolean arg)
{
    getConstraintEqualShapePtr()->equalTrim = arg.as_bool();
}



PyObject *ConstraintEqualShapePy::getCustomAttributes(const char* attr) const
{
    return ConstraintPy::getCustomAttributes(attr);
}

int ConstraintEqualShapePy::setCustomAttributes(const char* attr, PyObject* obj)
{
    return ConstraintPy::setCustomAttributes(attr, obj);
}
