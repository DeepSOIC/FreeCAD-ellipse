/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#ifndef _PreComp_
# include <BRepAlgoAPI_Cut.hxx>
#endif


#include "FeaturePartCut.h"

#include <Base/Exception.h>

using namespace Part;

PROPERTY_SOURCE(Part::Cut, Part::Boolean)


Cut::Cut(void)
{
}

BRepAlgoAPI_BooleanOperation* Cut::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    BRepAlgoAPI_BooleanOperation* op = new BRepAlgoAPI_Cut;
    TopTools_ListOfShape args; args.Append(base);
    TopTools_ListOfShape tools; tools.Append(tool);
    op->SetArguments(args);
    op->SetTools(tools);
    return op;
}
