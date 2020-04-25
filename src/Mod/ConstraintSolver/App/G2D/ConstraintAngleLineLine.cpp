#include "PreCompiled.h"

#include "ConstraintAngleLineLine.h"
#include "G2D/ConstraintAngleLineLinePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintAngleLineLine, FCS::G2D::ConstraintAngle);


ConstraintAngleLineLine::ConstraintAngleLineLine()
    : line1(nullptr), line2(nullptr)
{
    initAttrs();
}

void ConstraintAngleLineLine::initAttrs()
{
    ConstraintAngle::initAttrs();

    tieAttr_Shape(line1, "line1", ParaCurve::getClassTypeId());
    tieAttr_Shape(line2, "line2", ParaCurve::getClassTypeId());
}

PyObject* ConstraintAngleLineLine::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintAngleLineLinePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Base::DualNumber ConstraintAngleLineLine::calculateAngle(const ValueSet& vals) const
{
    Placement plm1 = line1->placement->value(vals);
    Placement plm2 = line2->placement->value(vals);
    Vector tang1 = (plm1 * line1->tshape().tangent(vals, 0.0)) ^ line1->reversed;
    Vector tang2 = (plm2 * line2->tshape().tangent(vals, 0.0)) ^ line2->reversed;
    return atan2(tang2.y, tang2.x) - atan2(tang1.y, tang1.x);
}
