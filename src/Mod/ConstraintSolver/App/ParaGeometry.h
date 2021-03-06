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

#ifndef FREECAD_CONSTRAINTSOLVER_PARAGEOMETRY_H
#define FREECAD_CONSTRAINTSOLVER_PARAGEOMETRY_H

#include "ParaObject.h"

namespace FCS {

class ParaGeometry;
typedef UnsafePyHandle<ParaGeometry> HParaGeometry;

class FCSExport ParaGeometry : public ParaObject
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public: //methods
    virtual HParaObject toShape() = 0;

    ///equal-shape constraint interface @{

        /**
         * @brief equalConstraintRank
         * @param geom2: the other shape
         * @param equalTrim: if true, matching of arc range is requested. If false, only full curves/surfaces should equal each other.
         * @return
         */
        virtual int equalConstraintRank(ParaGeometry& geom2, bool equalTrim) const;

        /**
         * @brief equalConstraintError should provide error functions that make the two geometries equal, i.e. it should be possible to make them coincident by translating and rotating one of them.
         * @param vals
         * @param returnbuf
         * @param geom2: the other shape
         * @param equalTrim: if true, matching of arc range is requested. If false, only full curves/surfaces should equal each other.
         */
        virtual void equalConstraintError(const ValueSet& vals, Base::DualNumber* returnbuf, ParaGeometry& geom2, bool equalTrim) const;

    ///@}
};

} //namespace

#endif
