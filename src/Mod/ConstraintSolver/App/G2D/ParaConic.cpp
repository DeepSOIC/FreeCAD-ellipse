#include "PreCompiled.h"

#include "ParaConic.h"
#include "G2D/ParaConicPy.h"
#include "G2D/ParaPointPy.h"

using namespace FCS;
using namespace FCS::G2D;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::G2D::ParaConic, FCS::G2D::ParaCurve);



PyObject* ParaConic::getPyObject()
{
    if (!_twin){
        _twin = new ParaConicPy(this);
        return _twin;
    } else  {
        return Py::new_reference_to(_twin);
    }
}

Position ParaConic::getFocus2(const ValueSet& vals) const
{
    Position f1 = getFocus1(vals);
    Position c = center->value(vals);
    return 2 * c  + -1 * f1; // = c - (f1-c)
}

int ParaConic::equalConstraintRank(ParaGeometry& geom2, bool equalTrim) const
{
    equalTrim = equalTrim && !this->isFull() && !static_cast<ParaCurve&>(geom2).isFull();
    return equalTrim ? 4 : 2;
}

void ParaConic::equalConstraintError(const ValueSet& vals, Base::DualNumber* returnbuf, ParaGeometry& geom2, bool equalTrim) const
{
    ParaConic& other = static_cast<ParaConic&>(geom2);
    equalTrim = equalTrim && !this->isFull() && other.isFull();

    DualNumber scaling = 1/this->getRMaj(vals);

    int i = 0;
    returnbuf[i++] = (getRMaj(vals) - other.getRMaj(vals)) * scaling;
    returnbuf[i++] = (getRMin(vals) - other.getRMin(vals)) * scaling;
    if (equalTrim) {
        returnbuf[i++] = vals[this->u0] - vals[other.u0];
        returnbuf[i++] = vals[this->u1] - vals[other.u1];
    }

}


