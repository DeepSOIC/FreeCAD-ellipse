#include "PreCompiled.h"

#include "ParaPoint.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE(FCS::G2D::ParaPoint, FCS::G2D::ParaGeometry2D);

Position ParaPoint::value(const ValueSet& vals) const
{
    return Position(vals[x], vals[y]);
}

ParaPoint::ParaPoint()
{
    initAttrs();
}

ParaPoint::ParaPoint(ParameterRef x, ParameterRef y)
    : ParaPoint()
{
    this->x = x;
    this->y = y;
}

void FCS::G2D::ParaPoint::initAttrs()
{
    ParaGeometry2D::initAttrs();

    tieAttr_Parameter(x, "x", true, 0.0);
    tieAttr_Parameter(y, "y", true, 0.0);
}

PyObject* ParaPoint::getPyObject()
{
    if (!_twin){
        new ParaPointPy(this);
        assert(_twin);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }

}
