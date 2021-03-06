/***************************************************************************
 *   Copyright (c) 2020 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLELINELINE_H
#define FREECAD_CONSTRAINTSOLVER_G2D_CONSTRAINTANGLELINELINE_H

#include "ConstraintAngle.h"

#include "ParaLine.h"

namespace FCS {
namespace G2D {

class ConstraintAngleLineLine;
typedef Base::UnsafePyHandle<ConstraintAngleLineLine> HConstraintAngleLineLine;

class FCSExport ConstraintAngleLineLine : public FCS::G2D::ConstraintAngle
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //data
    HShape_Line line1;
    HShape_Line line2;

public: //methods
    ConstraintAngleLineLine();
    ConstraintAngleLineLine(HShape_Line line1, HShape_Line line2, std::string label = "");

    void initAttrs() override;

    virtual PyObject* getPyObject() override;

public: //helpers
    ///computes true angle between tangent vectors of the curves
    Base::DualNumber calculateAngle(const ValueSet& vals) const override;

public: //friends
    friend class ConstraintAngleLineLinePy;
};

}} //namespace

#endif
