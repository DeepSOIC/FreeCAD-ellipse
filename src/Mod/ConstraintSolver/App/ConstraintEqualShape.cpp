#include "PreCompiled.h"

#include "ConstraintEqualShape.h"
#include "ConstraintEqualShapePy.h"

#include "ParaGeometryPy.h"

using namespace FCS;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::ConstraintEqualShape, FCS::Constraint);

ConstraintEqualShape::ConstraintEqualShape()
    : geom1(Py::None()), geom2(Py::None())
{
    initAttrs();
}

ConstraintEqualShape::ConstraintEqualShape(HParaGeometry geom1, HParaGeometry geom2, bool equalTrim)
    : ConstraintEqualShape()
{
    this->geom1 = geom1;
    this->geom2 = geom2;
    this->equalTrim = equalTrim;
}

void ConstraintEqualShape::initAttrs()
{
    Constraint::initAttrs();

    tieAttr_Child(geom1, "geom1", &ParaGeometryPy::Type);
    tieAttr_Child(geom2, "geom2", &ParaGeometryPy::Type);
}

void ConstraintEqualShape::throwIfIncomplete() const
{
    Constraint::throwIfIncomplete();
    typeCheck();
}

HParaObject ConstraintEqualShape::copy() const
{
    HConstraintEqualShape ret = Constraint::copy().downcast<ConstraintEqualShape>();
    ret->equalTrim = equalTrim;
    return ret;
}

int ConstraintEqualShape::rank() const
{
    return geom1->equalConstraintRank(*geom2, equalTrim);
}

void ConstraintEqualShape::error(const ValueSet& vals, Base::DualNumber* returnbuf) const
{
    return geom1->equalConstraintError(vals, returnbuf, *geom2, equalTrim);
}

PyObject* ConstraintEqualShape::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintEqualShapePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

void ConstraintEqualShape::typeCheck() const
{
    if (geom1->getTypeId() != geom2->getTypeId())
        throw Py::TypeError(std::string() + "geom1 and geom2 of ConstraintEqualShape must be of same type "
                                            "(got " + geom1->getTypeId().getName() + " and " +  geom2->getTypeId().getName() + ")");
}
