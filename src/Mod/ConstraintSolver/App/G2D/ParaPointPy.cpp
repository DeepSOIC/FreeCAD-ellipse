
// This file is generated by src/Tools/generateTemaplates/templateClassPyExport.py out of the .XML file
// Every change you make here gets lost in the next full rebuild!
// This File is normally built as an include in ParaPointPyImp.cpp! It's not intended to be in a project!

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>
#include <Base/PyObjectBase.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <CXX/Objects.hxx>

using Base::streq;
using namespace FCS::G2D;

/// Type structure of ParaPointPy
PyTypeObject ParaPointPy::Type = {
    PyVarObject_HEAD_INIT(&PyType_Type,0)
    "FCS::G2D.ParaPoint",     /*tp_name*/
    sizeof(ParaPointPy),                       /*tp_basicsize*/
    0,                                                /*tp_itemsize*/
    /* methods */
    PyDestructor,                                     /*tp_dealloc*/
    0,                                                /*tp_print*/
    0,                                                /*tp_getattr*/
    0,                                                /*tp_setattr*/
    0,                                                /*tp_compare*/
    __repr,                                           /*tp_repr*/
    0,                                                /*tp_as_number*/
    0,                                                /*tp_as_sequence*/
    0,                                                /*tp_as_mapping*/
    0,                                                /*tp_hash*/
    0,                                                /*tp_call */
    0,                                                /*tp_str  */
    __getattro,                                       /*tp_getattro*/
    __setattro,                                       /*tp_setattro*/
    /* --- Functions to access object as input/output buffer ---------*/
    0,                                                /* tp_as_buffer */
    /* --- Flags to define presence of optional/expanded features */
#if PY_MAJOR_VERSION >= 3
    Py_TPFLAGS_BASETYPE|Py_TPFLAGS_DEFAULT,        /*tp_flags */
#else
    Py_TPFLAGS_DEFAULT,        /*tp_flags */
#endif
    "2D point",           /*tp_doc */
    0,                                                /*tp_traverse */
    0,                                                /*tp_clear */
    0,                                                /*tp_richcompare */
    0,                                                /*tp_weaklistoffset */
    0,                                                /*tp_iter */
    0,                                                /*tp_iternext */
    FCS::G2D::ParaPointPy::Methods,                     /*tp_methods */
    0,                                                /*tp_members */
    FCS::G2D::ParaPointPy::GetterSetter,                     /*tp_getset */
    &FCS::ParaGeometryPy::Type,                        /*tp_base */
    0,                                                /*tp_dict */
    0,                                                /*tp_descr_get */
    0,                                                /*tp_descr_set */
    0,                                                /*tp_dictoffset */
    __PyInit,                                         /*tp_init */
    0,                                                /*tp_alloc */
    FCS::G2D::ParaPointPy::PyMake,/*tp_new */
    0,                                                /*tp_free   Low-level free-memory routine */
    0,                                                /*tp_is_gc  For PyObject_IS_GC */
    0,                                                /*tp_bases */
    0,                                                /*tp_mro    method resolution order */
    0,                                                /*tp_cache */
    0,                                                /*tp_subclasses */
    0,                                                /*tp_weaklist */
    0,                                                /*tp_del */
    0                                                 /*tp_version_tag */
#if PY_MAJOR_VERSION >= 3
    ,0                                                /*tp_finalize */
#endif
};

/// Methods structure of ParaPointPy
PyMethodDef ParaPointPy::Methods[] = {
    {NULL, NULL, 0, NULL}		/* Sentinel */
};



/// Attribute structure of ParaPointPy
PyGetSetDef ParaPointPy::GetterSetter[] = {
    {NULL, NULL, NULL, NULL, NULL}		/* Sentinel */
};




//--------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
ParaPointPy::ParaPointPy(ParaPoint *pcObject, PyTypeObject *T)
    : ParaGeometryPy(static_cast<ParaGeometryPy::PointerType>(pcObject), T)
{
    this->setShouldNotify(false);
}


//--------------------------------------------------------------------------
// destructor
//--------------------------------------------------------------------------
ParaPointPy::~ParaPointPy()                                // Everything handled in parent
{
    // delete the handled object when the PyObject dies
    ParaPointPy::PointerType ptr = static_cast<ParaPointPy::PointerType>(_pcTwinPointer);
    delete ptr;
}

//--------------------------------------------------------------------------
// ParaPointPy representation
//--------------------------------------------------------------------------
PyObject *ParaPointPy::_repr(void)
{
    return Py_BuildValue("s", representation().c_str());
}

//--------------------------------------------------------------------------
// ParaPointPy Attributes
//--------------------------------------------------------------------------
PyObject *ParaPointPy::_getattr(const char *attr)			// __getattr__ function: note only need to handle new state
{
    try {
        // getter method for special Attributes (e.g. dynamic ones)
        PyObject *r = getCustomAttributes(attr);
        if(r) return r;
    } // Please sync the following catch implementation with PY_CATCH
    catch(Base::AbortException &e)
    {
        e.ReportException();
        PyErr_SetObject(Base::BaseExceptionFreeCADAbort,e.getPyObject());
        return NULL;
    }
    catch(Base::Exception &e)
    {
        e.ReportException();
        auto pye = e.getPyExceptionType();
        if(!pye)
            pye = Base::BaseExceptionFreeCADError;
        PyErr_SetObject(pye,e.getPyObject());
        return NULL;
    }
    catch(std::exception &e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return NULL;
    }
    catch(const Py::Exception&)
    {
        // The exception text is already set
        return NULL;
    }
    catch(const char *e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e);
        return NULL;
    }
#ifndef DONT_CATCH_CXX_EXCEPTIONS
    catch(...)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,"Unknown C++ exception");
        return NULL;
    }
#endif

    PyMethodDef *ml = Methods;
    for (; ml->ml_name != NULL; ml++) {
        if (attr[0] == ml->ml_name[0] &&
            strcmp(attr+1, ml->ml_name+1) == 0)
            return PyCFunction_New(ml, this);
    }

    PyErr_Clear();
    return ParaGeometryPy::_getattr(attr);
}

int ParaPointPy::_setattr(const char *attr, PyObject *value) // __setattr__ function: note only need to handle new state
{
    try {
        // setter for special Attributes (e.g. dynamic ones)
        int r = setCustomAttributes(attr, value);
        // r = 1: handled
        // r = -1: error
        // r = 0: ignore
        if (r == 1)
            return 0;
        else if (r == -1)
            return -1;
    } // Please sync the following catch implementation with PY_CATCH
    catch(Base::AbortException &e)
    {
        e.ReportException();
        PyErr_SetObject(Base::BaseExceptionFreeCADAbort,e.getPyObject());
        return -1;
    }
    catch(Base::Exception &e)
    {
        e.ReportException();
        auto pye = e.getPyExceptionType();
        if(!pye)
            pye = Base::BaseExceptionFreeCADError;
        PyErr_SetObject(pye,e.getPyObject());
        return -1;
    }
    catch(std::exception &e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e.what());
        return -1;
    }
    catch(const Py::Exception&)
    {
        // The exception text is already set
        return -1;
    }
    catch(const char *e)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,e);
        return -1;
    }
#ifndef DONT_CATCH_CXX_EXCEPTIONS
    catch(...)
    {
        PyErr_SetString(Base::BaseExceptionFreeCADError,"Unknown C++ exception");
        return -1;
    }
#endif

    return ParaGeometryPy::_setattr(attr, value);
}

ParaPoint *ParaPointPy::getParaPointPtr(void) const
{
    return static_cast<ParaPoint *>(_pcTwinPointer);
}

#if 0
/* From here on come the methods you have to implement, but NOT in this module. Implement in ParaPointPyImp.cpp! This prototypes 
 * are just for convenience when you add a new method.
 */

PyObject *ParaPointPy::PyMake(struct _typeobject *, PyObject *, PyObject *)  // Python wrapper
{
    // create a new instance of ParaPointPy and the Twin object 
    return new ParaPointPy(new ParaPoint);
}

// constructor method
int ParaPointPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}


// returns a string which represents the object e.g. when printed in python
std::string ParaPointPy::representation(void) const
{
    return std::string("<ParaPoint object>");
}



PyObject *ParaPointPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int ParaPointPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
#endif



