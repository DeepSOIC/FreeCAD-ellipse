#/***************************************************************************
# *   Copyright (c) Victor Titov (DeepSOIC)                                 *
# *                                           (vv.titov@gmail.com) 2016     *
# *                                                                         *
# *   This file is part of the FreeCAD CAx development system.              *
# *                                                                         *
# *   This library is free software; you can redistribute it and/or         *
# *   modify it under the terms of the GNU Library General Public           *
# *   License as published by the Free Software Foundation; either          *
# *   version 2 of the License, or (at your option) any later version.      *
# *                                                                         *
# *   This library  is distributed in the hope that it will be useful,      *
# *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
# *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
# *   GNU Library General Public License for more details.                  *
# *                                                                         *
# *   You should have received a copy of the GNU Library General Public     *
# *   License along with this library; see the file COPYING.LIB. If not,    *
# *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
# *   Suite 330, Boston, MA  02111-1307, USA                                *
# *                                                                         *
# ***************************************************************************/

__title__ = "BOPTools package"
__url__ = "http://www.freecadweb.org"
__doc__ = """BOPTools Package (part of FreeCAD). Routines that power Connect, Embed, Cutout,
BooleanFragments and Slice features of Part Workbench. Useful for other custom BOP-like 
operations"""

__all__ = [
"GeneralFuseResult",
"JoinAPI",
"JoinFeatures",
"ShapeMerge",
"Utils",
"SplitFeatures",
]

def importAll():
    from . import GeneralFuseResult
    from . import JoinAPI
    from . import JoinFeatures
    from . import ShapeMerge
    from . import Utils
    from . import SplitFeatures

def reloadAll():
    for modstr in __all__:
        reload(globals()[modstr])
        
def addCommands():
    JoinFeatures.addCommands()
    SplitFeatures.addCommands()