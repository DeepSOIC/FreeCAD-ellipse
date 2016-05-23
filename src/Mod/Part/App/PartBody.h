/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)       <vv.titov@gmail.com> *
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


#ifndef PART_Body_H
#define PART_Body_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>
#include "BodyBase.h"

namespace Part
{
/**
 * @brief Part::Body is a document object that gathers a set of Part workbench
 * and other workbench features derived from Part::Feature. It exposes the final
 * shape, as pointed by Tip property, to the outside world as its Shape
 * property.
 */
class PartExport Body : public BodyBase
{
    PROPERTY_HEADER(Part::Body);

public:
    Body();

};

} //namespace Part


#endif // PART_Body_H
