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

__title__="BOPTools.Utils module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecadweb.org"
__doc__ = "Utility code, used by various modules of BOPTools."

class HashableShape(object):
    "Decorator for Part.Shape, that can be used as key in dicts"
    def __init__(self, shape):
        self.Shape = shape
        self.hash = shape.hashCode()

    def __eq__(self, other):
        return self.Shape.isSame(other.Shape)

    def __hash__(self):
        return self.hash

class HashableShape_Deep(object):
    def __init__(self, shape):
        self.Shape = shape
        self.hash = 0
        for el in shape.Vertexes + shape.Edges + shape.Faces:
            self.hash = self.hash ^ el.hashCode()
    
    def __eq__(self, other):
        # avoiding extensive comparison for now. Just doing a few extra tests should reduce the already-low chances of false-positives
        if self.hash == other.hash:
            if len(self.Shape.Vertexes) == len(other.Shape.Vertexes):
                if self.Shape.ShapeType == other.Shape.ShapeType:
                    return True
        return False    
        
    def __hash__(self):
        return self.hash


# adapted from http://stackoverflow.com/a/3603824/6285007
class FrozenClass(object):
    '''FrozenClass: prevents adding new attributes to class outside of __init__'''
    __isfrozen = False
    def __setattr__(self, key, value):
        if self.__isfrozen and not hasattr(self, key):
            raise TypeError( "{cls} has no attribute {attr}".format(cls= self.__class__.__name__, attr= key) )
        object.__setattr__(self, key, value)

    def _freeze(self):
        self.__isfrozen = True

    def _unfreeze(self):
        self.__isfrozen = False

