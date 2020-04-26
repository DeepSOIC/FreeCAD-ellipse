#include "PreCompiled.h"

#include "ParaCircle.h"
#include "G2D/ParaCirclePy.h"
#include "PyUtils.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaCircle, FCS::G2D::ParaCurve);

ParaCircle::ParaCircle(bool full)
    : center(Py::None())
{
    _isFull = full;
    initAttrs();
}

void FCS::G2D::ParaCircle::initAttrs()
{
    ParaCurve::initAttrs();

    tieAttr_Child(center, "center", &ParaPointPy::Type, true);
    //                                , make, req., defv
    tieAttr_Parameter(radius, "radius", true, true, 5.0);
}

std::vector<ParameterRef> ParaCircle::makeParameters(HParameterStore into)
{
    bool init_u0 = !isFull() && u0.isNull();
    bool init_u1 = !isFull() && u1.isNull();
    bool init_p0 = !isFull() && p0.isNone();
    bool init_p1 = !isFull() && p1.isNone();
    std::vector<ParameterRef> ret = ParaCurve::makeParameters(into);
    if (init_u0){
        u0.savedValue() = 0.0;
    }
    if (init_u1){
        u1.savedValue() = 0.25*TURN;
    }
    if (init_p0 && p0->isComplete()){
        p0->x.savedValue() = 5;
        p0->y.savedValue() = 0;
    }
    if (init_p1 && p1->isComplete()){
        p1->x.savedValue() = 0;
        p1->y.savedValue() = 5;
    }
    return ret;
}

Position ParaCircle::value(const ValueSet& vals, DualNumber u)
{
    return center->value(vals) + vals[radius] * Vector(cos(u), sin(u)) ;
}

Vector ParaCircle::tangent(const ValueSet& vals, DualNumber u)
{
    return vals[radius] * Vector(-sin(u), cos(u));
}

Vector ParaCircle::tangentAtXY(const ValueSet& vals, Position p)
{
    return (p - center->value(vals)).rotate90ccw();
}

DualNumber ParaCircle::length(const ValueSet& vals, DualNumber u0, DualNumber u1) const
{
    return vals[radius] * positiveAngle(u1-u0);
}

DualNumber ParaCircle::length(const ValueSet& vals) const
{
    if (isFull())
        return TURN * vals[radius];
    else
        return vals[radius] * positiveAngle(vals[u1] - vals[u0]);
}

DualNumber ParaCircle::pointOnCurveErrFunc(const ValueSet& vals, Position p)
{
    return (p - center->value(vals)).length() - vals[radius];
}

int ParaCircle::equalConstraintRank(ParaGeometry& geom2, bool equalTrim) const
{
    equalTrim = equalTrim && !this->isFull() && !static_cast<ParaCurve&>(geom2).isFull();
    return equalTrim ? 3 : 1;
}

void ParaCircle::equalConstraintError(const ValueSet& vals, Base::DualNumber* returnbuf, ParaGeometry& geom2, bool equalTrim) const
{
    ParaCircle& other = static_cast<ParaCircle&>(geom2);
    equalTrim = equalTrim && !this->isFull() && other.isFull();
    int i = 0;
    returnbuf[i++] = (vals[this->radius] - vals[other.radius]) / radius.masterScale();
    if (equalTrim) {
        returnbuf[i++] = vals[this->u0] - vals[other.u0];
        returnbuf[i++] = vals[this->u1] - vals[other.u1];
    }
}

PyObject* ParaCircle::getPyObject()
{
    if (!_twin){
        _twin = new ParaCirclePy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}


