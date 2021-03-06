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

#ifndef FREECAD_CONSTRAINTSOLVER_G2D_PARAGEOMETRY2D_H
#define FREECAD_CONSTRAINTSOLVER_G2D_PARAGEOMETRY2D_H

#include <Mod/ConstraintSolver/App/ParaGeometry.h>
#include "ParaShape.h"

namespace FCS {
namespace G2D {

class ParaGeometry2D;

template <  typename TGeo2D,
            typename = typename std::enable_if<
                    std::is_base_of<ParaGeometry2D, typename std::decay<TGeo2D>::type>::value
             >::type
>
UnsafePyHandle<ParaShape<TGeo2D>> toDShape(UnsafePyHandle<TGeo2D> t) {
    // to be improved:
    // 1. with singleton identify transformation
    // 2. or with templated ParaShape including a identity transformation
    return (new ParaShape<TGeo2D>((new ParaTransform())->getHandle<ParaTransform>(), t))-> template getHandle<ParaShape<TGeo2D>>();
}

template <  typename TGeo2D,
            typename = typename std::enable_if<
                    std::is_base_of<ParaGeometry2D, typename std::decay<TGeo2D>::type>::value
             >::type
>
UnsafePyHandle<ParaShape<TGeo2D>> toDShape(TGeo2D t) {
    // to be improved:
    // 1. with singleton identify transformation
    // 2. or with templated ParaShape including a identity transformation
    return (new ParaShape<TGeo2D>((new ParaTransform())->getHandle<ParaTransform>(), t. template getHandle<TGeo2D>()))-> template getHandle<ParaShape<TGeo2D>>();
}

    
typedef UnsafePyHandle<ParaGeometry2D> HParaGeometry2D;

class FCSExport ParaGeometry2D : public FCS::ParaGeometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public://data

public://methods
    virtual PyObject* getPyObject() override;
    virtual HParaObject toShape() override;

public: //friends
    friend class ParaGeometry2DPy;

};




}} //namespace

#endif
