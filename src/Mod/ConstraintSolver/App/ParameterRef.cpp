#include "PreCompiled.h"

#include "ParameterRef.h"
#include "ParameterRefPy.h"
#include <Base/Exception.h>

using namespace FCS;

ParameterRef::ParameterRef(HParameterStore st, int index)
    : _store(st), _ownIndex(index)
{
    if(index < 0 || index > st->size())
        throw Base::ValueError("Parameter index is out of bounds");
}

ParameterRef::~ParameterRef()
{
}

int ParameterRef::masterIndex() const {
    return _store->_params[_ownIndex].masterIndex();
}

bool ParameterRef::isMaster() const
{
    return ownIndex() == masterIndex();
}

ParameterRef ParameterRef::master() const
{
    return ParameterRef(this->host(), this->masterIndex());
}

double& ParameterRef::ownSavedValue() const {
    return _store->_params[_ownIndex].savedValue;
}

double& ParameterRef::savedValue() const {
    return _store->_params[masterIndex()].savedValue;
}

double& ParameterRef::ownScale() const {
    return _store->_params[_ownIndex].scale;
}

double& ParameterRef::masterScale() const {
    return _store->_params[masterIndex()].scale;
}

bool& ParameterRef::ownFixed() const
{
    return param().fixed;
}

bool ParameterRef::isFixed() const
{
    return master().param().fixed;
}

void ParameterRef::fix()
{
    host()->fix(*this);
}

Parameter& ParameterRef::param() const {
    return _store->_params[_ownIndex];
}

ParameterRef ParameterRef::masterParam() const {
    return ParameterRef(_store, masterIndex());
}

bool ParameterRef::isSameRef(const ParameterRef& other) const {
    return &(param()) == &(other.param());
}

bool ParameterRef::isSameValue(const ParameterRef& other) const {
    return (_store == other._store)
            && (masterIndex() == other.masterIndex());
}

UnsafePyHandle<ParameterRef> ParameterRef::getPyHandle() const
{
    UnsafePyHandle<ParameterRef> ret(new ParameterRefPy(new ParameterRef(*this)), /*new_reference=*/true);
    return ret;
}

PyObject* ParameterRef::getPyObject() const
{
    return Py::new_reference_to(getPyHandle());
}
