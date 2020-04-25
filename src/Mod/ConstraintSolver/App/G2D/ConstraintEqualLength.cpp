#include "PreCompiled.h"

#include "ConstraintEqualLength.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintEqualLengthPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintEqualLength, FCS::SimpleConstraint);


ConstraintEqualLength::ConstraintEqualLength()
    : crv1(Py::None()), crv2(Py::None())
{
    initAttrs();
}

void ConstraintEqualLength::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(crv1, "crv1", ParaPoint::getClassTypeId());
    tieAttr_Shape(crv2, "crv2", ParaPoint::getClassTypeId());
}

Base::DualNumber ConstraintEqualLength::error1(const ValueSet& vals) const
{
    return crv2->tshape().length(vals) - crv1->tshape().length(vals);
}

void ConstraintEqualLength::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintEqualLength::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintEqualLengthPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
