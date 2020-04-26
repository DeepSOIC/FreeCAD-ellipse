#include "PreCompiled.h"

#include "ParaGeometry.h"
#include "ParaGeometryPy.h"

using namespace FCS;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::ParaGeometry, FCS::ParaObject);

int ParaGeometry::equalConstraintRank(ParaGeometry&, bool) const
{
    return 0;
}

void ParaGeometry::equalConstraintError(const ValueSet&, Base::DualNumber*, ParaGeometry&, bool) const
{

}
