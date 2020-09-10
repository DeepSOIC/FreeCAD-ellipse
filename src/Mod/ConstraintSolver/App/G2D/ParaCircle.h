/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
#pragma once //to make qt creator happy, see QTCREATORBUG-20883

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARACIRCLE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARACIRCLE_H

#include "ParaCurve.h"
#include "ParaPoint.h"

namespace FCS {
namespace G2D {

class ParaCircle;
typedef Base::UnsafePyHandle<ParaCircle> HParaCircle;
typedef Base::UnsafePyHandle<ParaShape<ParaCircle>> HShape_Circle;

class FCSExport ParaCircle : public FCS::G2D::ParaCurve
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    HParaPoint center;
    ParameterRef radius;

public://methods
    ParaCircle(bool full = true);
    static HParaCircle makeArc(){return new ParaCircle(false);}
    void initAttrs() override;
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into) override;
    virtual PyObject* getPyObject() override;

    virtual Position value(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangent(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangentAtXY(const ValueSet& vals, Position p) override;
    virtual bool supports_tangentAtXY() override {return true;}

    virtual DualNumber length(const ValueSet& vals, DualNumber u0, DualNumber u1) const override;
    virtual DualNumber length(const ValueSet& vals) const override;
    virtual bool supports_length() override {return true;}

    virtual DualNumber pointOnCurveErrFunc(const ValueSet& vals, Position p) override;
    virtual bool supports_pointOnCurveErrFunc() override {return true;}

    virtual int equalConstraintRank(ParaGeometry& geom2, bool equalTrim) const override;
    virtual void equalConstraintError(const ValueSet& vals, Base::DualNumber* returnbuf, ParaGeometry& geom2, bool equalTrim) const override;

public: //friends
    friend class ParaCirclePy;

};

}} //namespace

#endif
