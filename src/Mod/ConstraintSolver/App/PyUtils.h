#ifndef FREECAD_CONSTRAINTSOLVER_PYUTILS_H
#define FREECAD_CONSTRAINTSOLVER_PYUTILS_H

#ifndef BaseExport
    #define BaseExport
#endif

#include <CXX/Objects.hxx>
#include <Base/DualNumber.h>
#include <cmath>
#include <Base/Exception.h>
#include <Base/PyObjectBase.h>


namespace FCS {

///converts std::vector (or whatever container) to Py::List. The elements must have getPyObject method.
template<class Vec>
inline Py::List asPyList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret;
    for (auto &v : vec) {
        ret.append(Py::Object(v.getPyObject(), true));
    }
    return ret;
}

///can convert vector of doubles to a py list of floats for example (use Py::Fload as PyCxxConstruct argument)
template<class Vec, class PyCxxConstruct>
inline Py::List asPyList(Vec& vec){ //vec is usually not changed... unless getPyObject changes the object, which does happen to some
    Py::List ret(vec.size());
    for (int i = 0; i < vec.size(); ++i) {
        ret[i] = PyCxxConstruct(vec[i]);
    }
    return ret;
}

inline Py::Tuple pyDualNumber(Base::DualNumber v){
    Py::Tuple tup(2);
    tup[0] = Py::Float(v.re);
    tup[1] = Py::Float(v.du);
    return tup;
}

//temporary replacement for PyCXX's Object:::setAttr, that doesn't absorb the original error, for until PyCXX is uptated
inline void setAttr(Py::Object obj, std::string attrname, Py::Object value)
{
    if( PyObject_SetAttrString( obj.ptr(), const_cast<char*>( attrname.c_str() ), value.ptr() ) == -1 )
    {
        throw Py::Exception();
    }
}

template<class Ty>
inline Ty* pyTypeCheck(PyObject* obj)
{
    if (! PyObject_TypeCheck(obj, &Ty::Type)){
        std::stringstream ss;
        ss << "Expected " << Ty::Type.tp_name
           << " but got " << Py::Object(obj).type().as_string();
        throw Py::TypeError(ss.str());
    }
    return static_cast<Ty*>(obj);
};

inline PyObject* raiseBaseException(Base::Exception& e)
{
    auto pye = e.getPyExceptionType();
    if(!pye)
        pye = Base::BaseExceptionFreeCADError;
    PyErr_SetObject(pye, e.getPyObject());
    return nullptr;
}

} //namespace

#endif
