/***************************************************************************
 *   Copyright (c) Ajinkya Dahale       (ajinkyadahale@gmail.com) 2017     *
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

#include "PreCompiled.h"

#include <TopTools_ListOfShape.hxx>

#include "TopoHistory.h"
#include "TopoShape.h"

using namespace Part;

TYPESYSTEM_SOURCE(Part::TopoHistory, Base::BaseClass);

TopoHistory::TopoHistory()
{

}

TopTools_ListOfShape TopoHistory::modified(TopoShape oldShape)
{
    if (this->modShapeMaker.get()) {
        TopoDS_Shape _shape = oldShape.getShape();
        return this->modShapeMaker->Modified(_shape);
    }
    return TopTools_ListOfShape();
}

TopTools_ListOfShape TopoHistory::generated(TopoShape oldShape)
{
    if (this->modShapeMaker.get()) {
        TopoDS_Shape _shape = oldShape.getShape();
        return this->modShapeMaker->Generated(_shape);
    }
    return TopTools_ListOfShape();
}


bool TopoHistory::isDeleted(TopoShape oldShape)
{
    if (this->modShapeMaker.get()) {
        TopoDS_Shape _shape = oldShape.getShape();
        return this->modShapeMaker->IsDeleted(_shape);
    }
    return false;
}
