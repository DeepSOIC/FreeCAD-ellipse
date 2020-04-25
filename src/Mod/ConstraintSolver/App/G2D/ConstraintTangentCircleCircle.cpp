#include "PreCompiled.h"

#include "ConstraintTangentCircleCircle.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintTangentCircleCirclePy.h"

#include "DualMath.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintTangentCircleCircle, FCS::SimpleConstraint);


ConstraintTangentCircleCircle::ConstraintTangentCircleCircle()
    : circle1(Py::None()), circle2(Py::None())
{
    initAttrs();
    setReversed(true);
}

void ConstraintTangentCircleCircle::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(circle1, "circle1", ParaCircle::getClassTypeId());
    tieAttr_Shape(circle2, "circle2", ParaCircle::getClassTypeId());
}

Base::DualNumber ConstraintTangentCircleCircle::error1(const ValueSet& vals) const
{
    Position c1 = circle1->placement->value(vals) * circle1->tshape().center->value(vals);
    Position c2 = circle2->placement->value(vals) * circle2->tshape().center->value(vals);
    DualNumber r1 = vals[circle1->tshape().radius] ^ circle1->reversed;
    DualNumber r2 = vals[circle2->tshape().radius] ^ circle2->reversed;
    return (c1 - c2).length() - abs(r1 - r2 * _revers);
}

void ConstraintTangentCircleCircle::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintTangentCircleCircle::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintTangentCircleCirclePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
