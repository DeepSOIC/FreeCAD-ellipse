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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARAELLIPSE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARAELLIPSE_H

#include "ParaConic.h"

namespace FCS {
namespace G2D {

class ParaEllipse;
typedef Base::UnsafePyHandle<ParaEllipse> HParaEllipse;
typedef Base::UnsafePyHandle<ParaShape<ParaEllipse>> HShape_Ellipse;

class FCSExport ParaEllipse : public FCS::G2D::ParaConic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data
    bool _bare = true;

public://methods
    ParaEllipse(bool full = false, bool bare = false);
    ///makes bare-bones (minimum construction geometry). Construction geometry is still supported, but you have to manually fill it in before making rule constraints
    static HParaEllipse makeBare(){return new ParaEllipse(true, true);};
    ///makes fully equipped (with diameters and both foci)
    static HParaEllipse makePosh(){return new ParaEllipse(true, false);};
    ///makes bare-bones arc (minimum construction geometry)
    static HParaEllipse makeBareArc(){return new ParaEllipse(false, true);};
    ///makes fully equipped arc (with diameters and both foci)
    static HParaEllipse makePoshArc(){return new ParaEllipse(false, false);};

    void initAttrs() override;
    virtual std::vector<ParameterRef> makeParameters(HParameterStore into) override;
    virtual PyObject* getPyObject() override;

    virtual Position value(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangent(const ValueSet& vals, DualNumber u) override;
    virtual Vector tangentAtXY(const ValueSet& vals, Position p) override;
    virtual bool supports_tangentAtXY() override {return true;}

    //virtual DualNumber length(const ValueSet& vals, DualNumber u0, DualNumber u1) override;
    //virtual DualNumber length(const ValueSet& vals) override;
    //virtual bool supports_length() override {return true;}

    virtual DualNumber pointOnCurveErrFunc(const ValueSet& vals, Position p) override;
    virtual bool supports_pointOnCurveErrFunc() override {return true;}

    //ParaEllipse does not need rule constraints for endpoints, since it uses its very endpoints to define itself.
    virtual std::vector<HConstraint> makeRuleConstraints() override;

    virtual Position getFocus1(const ValueSet&) const override;
    virtual DualNumber getF(const ValueSet&) const override;
    virtual DualNumber getRMaj(const ValueSet&) const override;
    virtual DualNumber getRMin(const ValueSet&) const override;

public: //friends
    friend class ParaEllipsePy;

};

}} //namespace

#endif
