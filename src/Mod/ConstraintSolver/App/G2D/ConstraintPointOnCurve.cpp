#include "PreCompiled.h"

#include "ConstraintPointOnCurve.h"
#include "src/Mod/ConstraintSolver/App/G2D/ConstraintPointOnCurvePy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ConstraintPointOnCurve, FCS::SimpleConstraint);


ConstraintPointOnCurve::ConstraintPointOnCurve()
    : crv(Py::None()), p(Py::None())
{
    initAttrs();
}

ConstraintPointOnCurve::ConstraintPointOnCurve(HShape_Point p, HShape_Curve crv)
    : crv(crv), p(p)
{
    initAttrs();
}

void ConstraintPointOnCurve::initAttrs()
{
    SimpleConstraint::initAttrs();

    tieAttr_Shape(crv, "crv");
    tieAttr_Shape(p, "p");
}

Base::DualNumber ConstraintPointOnCurve::error1(const ValueSet& vals) const
{
    Position pv = p->placement->value(vals) * p->tshape()(vals);
    Placement plm_crv = crv->placement->value(vals);
    Position pvl = plm_crv.inverse() * pv; // position of point in local cs of curve
    return crv->tshape().pointOnCurveErrFunc(vals, pvl);
}

void ConstraintPointOnCurve::setWeight(double weight)
{
    Constraint::setWeight(weight);
    scale = weight / _sz.avgElementSize;
}

PyObject* ConstraintPointOnCurve::getPyObject()
{
    if (!_twin){
        _twin = new ConstraintPointOnCurvePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}
