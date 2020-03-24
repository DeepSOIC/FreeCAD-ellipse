#include "Constraint.h"
#include "DualMath.h"

using namespace FCS;
using DualNumber = Base::DualNumber;

TYPESYSTEM_SOURCE_ABSTRACT(FCS::Constraint, FCS::ParaObject);

double Constraint::maxStep(const ValueSet& /*vals*/, const ValueSet& /*dir*/) const
{
    return 1e12;
}

void Constraint::setWeight(double weight)
{
    _weight = weight;
    scale = weight;
}

void Constraint::setProblemSize(ProblemSizeInfo sz)
{
    _sz = sz;
    setWeight(_weight);//to recompute scale
}

std::vector<ParameterRef> Constraint::datumParameters() const
{
    return std::vector<ParameterRef>();
}

std::vector<double> Constraint::caluclateDatum()
{
    return std::vector<double>();
}

Base::DualNumber Constraint::netError(const ValueSet& on) const
{
    return sqrt(netSqError(on));
}

Base::DualNumber Constraint::netSqError(const ValueSet& on) const
{
    std::vector<DualNumber> buf(rank()); //#FIXME: use stack-allocated vector, boost has some
    error(on, buf.data());

    if (rank() == 1)
        return sq(buf[0]);

    DualNumber acc;
    for(DualNumber v : buf){
        acc = acc + sq(v);
    }
    return acc;
}

double Constraint::netError() const
{
    if (parameters().size() == 0)
        throw Base::RuntimeError("constraint is not initialized or updated");

    return netError(parameters()[0].host()->asValueSet()).re;
}

HParaObject Constraint::copy() const
{
    HConstraint ret = ParaObject::copy();
    ret->_weight = _weight;
    ret->_sz =  _sz;
    ret->scale = scale;
    ret->priority = priority;
    return ret;
}
