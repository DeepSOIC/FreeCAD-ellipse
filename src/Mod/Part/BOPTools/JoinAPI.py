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

__title__="BOPTools.JoinAPI module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecadweb.org"
__doc__ = "JoinFeatures functions that operate on shapes."

import Part
from . import ShapeMerge
from .GeneralFuseResult import GeneralFuseResult


def fuse(list_of_shapes):
    """fuse(list_of_shapes): replica of Part.Shape's multiFuse method done via generalFuse. 
    Made for testing and example purposes."""
    if len(list_of_shapes) < 2:
        return Part.makeCompound(list_of_shapes)
    pieces, map = list_of_shapes[0].generalFuse(list_of_shapes[1:])
    return ShapeMerge.mergeShapes(pieces.childShapes())

def shapeOfMaxSize(list_of_shapes):
    #first, check if shapes can be compared by size
    ShapeMerge.dimensionOfShapes(list_of_shapes)
    
    #find it!
    max_size = -1e100 # max size encountered
    count_max = 0 # number of shapes with size equal to max_size
    shape_max = None # shape of max_size
    for sh in list_of_shapes:
        v = abs(Part.cast_to_shape(sh).Mass)
        if v > max_size + 1e-8 :
            max_size = v
            shape_max = sh
            count_max = 1
        elif abs(v - max_size) <= 1e-8 :
            count_max = count_max + 1
    if count_max > 1 :
        raise ValueError("There is more than one largest piece!")
    return shape_max

def compound_leaves(shape_or_compound):
    if shape_or_compound.ShapeType == "Compound":
        leaves = []
        for child in shape_or_compound.childShapes():
            leaves.extend( compound_leaves(child) )
        return leaves
    else:
        return [shape_or_compound]
    
def connect(list_of_shapes):
    """connect(list_of_shapes): connects solids (walled objects), shells and wires by throwing 
    off small parts that result when splitting them at intersections. 
    
    Compounds in list_of_shapes are automatically exploded, so self-intersecting compounds 
    are valid for connect."""
    
    # unfortunately, GFA doesn't return map info when compounds are supplied. So, we have to explode all compounds before GFA.
    new_list_of_shapes = []
    for sh in list_of_shapes:
        new_list_of_shapes.extend( compound_leaves(sh) )
    list_of_shapes = new_list_of_shapes
    
    #test if shapes are compatible for connecting
    dim = ShapeMerge.dimensionOfShapes(list_of_shapes)
    if dim == 0:
        raise TypeError("Cannot connect vertices!")
        
    if len(list_of_shapes) < 2:
        return Part.makeCompound(list_of_shapes)
    
    # collect shape types and unify if possible (e.g. if we have a mix of edges and wires, make all into wires.
    types = set()
    for source_shape in list_of_shapes:
        types.add(source_shape.ShapeType)
    if "CompSolid" in types:
        raise TypeError("Cannot connect compsolids (yet)")
    listy_types = set(["Wire","Shell","CompSolid","Compound"])
    nonlisty_types = set(["Vertex","Edge","Face","Solid"])
    if not types.issubset(nonlisty_types):
        for i in range(len(list_of_shapes)):
            if list_of_shapes[i].ShapeType == "Edge":
                list_of_shapes[i] = Part.Wire([list_of_shapes[i]])
            elif list_of_shapes[i].ShapeType == "Face":
                list_of_shapes[i] = Part.Shell([list_of_shapes[i]])
            elif list_of_shapes[i].ShapeType == "Solid":
                list_of_shapes[i] = Part.CompSolid([list_of_shapes[i]])

    pieces, map = list_of_shapes[0].generalFuse(list_of_shapes[1:])
    ao = GeneralFuseResult(list_of_shapes, (pieces, map))
    if not types.issubset(nonlisty_types):
        ao.splitWiresShells()
    print len(ao.pieces)," pieces total"
    
    keepers = []
    all_danglers = [] # debug
                
    #add all biggest dangling pieces
    for src in ao.source_shapes:
        danglers = [piece for piece in ao.piecesFromSource(src) if len(ao.sourcesOfPiece(piece)) == 1]
        all_danglers.extend(danglers)
        largest = shapeOfMaxSize(danglers)
        if largest is not None:
            keepers.append(largest)

    touch_test_list = Part.Compound(keepers)
    #add all intersection pieces that touch danglers, triple intersection pieces that touch duals, and so on
    for ii in range(2, ao.largestOverlapCount()+1):
        list_ii_pieces = [piece for piece in ao.pieces if len(ao.sourcesOfPiece(piece)) == ii]
        keepers_2_add = []
        for piece in list_ii_pieces: 
            if ShapeMerge.isConnected(piece, touch_test_list):
                keepers_2_add.append(piece)
        if len(keepers_2_add) == 0:
            break
        keepers.extend(keepers_2_add)
        touch_test_list = Part.Compound(keepers_2_add)
        
    
    #merge, and we are done!
    print len(keepers)," pieces to keep"
    return ShapeMerge.mergeShapes(keepers)

def embed(shape_base, shape_tool):
    # using legacy implementation, except adding support for shells
    pieces = compound_leaves(shape_base.cut(shape_tool))
    piece = shapeOfMaxSize(pieces)
    result = piece.fuse(shape_tool)
    dim = ShapeMerge.dimensionOfShapes(pieces)
    if dim == 2:
        # fusing shells returns shells that are still split. Reassemble them
        result = ShapeMerge.mergeShapes(result.Faces)
    elif dim == 1:
        result = ShapeMerge.mergeShapes(result.Edges)
    return result
    
def cutout(shape_base, shape_tool):
    #if base is multi-piece, work on per-piece basis
    shapes_base = compound_leaves(shape_base)
    if len(shapes_base) > 1:
        result = []
        for sh in shapes_base:
            result.append(cutout(sh, shape_tool))
        return Part.Compound(result)
    
    shape_base = shapes_base[0]
    pieces = compound_leaves(shape_base.cut(shape_tool))
    return shapeOfMaxSize(pieces)
