#include "PreCompiled.h"

#include "ConstraintVertical.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintVerticalPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintVertical, FCS::SimpleConstraint);


ConstraintVertical::ConstraintVertical()
    : p1(Py::None()), p2(Py::None())
{
    initAttrs();
}

ConstraintVertical::ConstraintVertical(HShape_Point p1, HShape_Point p2)
    : p1(p1), p2(p2)
{
    initAttrs();
}

void ConstraintVertical::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(p1, "p1", ParaPoint::getClassTypeId());
    tieAttr_Shape(p2, "p2", ParaPoint::getClassTypeId());
}

Base::DualNumber ConstraintVertical::error1(const ValueSet& vals) const
{
    Position p1v = p1->placement->value(vals) * p1->tshape()(vals);
    Position p2v = p2->placement->value(vals) * p2->tshape()(vals);
    Vector diff = p1v - p2v;
    return diff.x;
}

void ConstraintVertical::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintVertical::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintVerticalPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
