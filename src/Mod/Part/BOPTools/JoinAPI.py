#***************************************************************************
#*                                                                         *
#*   Copyright (c) 2016 - Victor Titov (DeepSOIC)                          *
#*                                               <vv.titov@gmail.com>      *  
#*                                                                         *
#*   This program is free software; you can redistribute it and/or modify  *
#*   it under the terms of the GNU Lesser General Public License (LGPL)    *
#*   as published by the Free Software Foundation; either version 2 of     *
#*   the License, or (at your option) any later version.                   *
#*   for detail see the LICENCE text file.                                 *
#*                                                                         *
#*   This program is distributed in the hope that it will be useful,       *
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
#*   GNU Library General Public License for more details.                  *
#*                                                                         *
#*   You should have received a copy of the GNU Library General Public     *
#*   License along with this program; if not, write to the Free Software   *
#*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
#*   USA                                                                   *
#*                                                                         *
#***************************************************************************

__doc__ = "JoinFeatures functions that operate on shapes. Useful for programming."

import Part
import ShapeMerge


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
    
    # unfortunately, GFA doesn't return map info when compounds are supplied. So, we have to explode all compounds before GFA.
    new_list_of_shapes = []
    for sh in list_of_shapes:
        new_list_of_shapes.extend( compound_leaves(sh) )
    list_of_shapes = new_list_of_shapes
        
    if len(list_of_shapes) < 2:
        return Part.makeCompound(list_of_shapes)
    pieces, map = list_of_shapes[0].generalFuse(list_of_shapes[1:])
    pieces = pieces.childShapes()
    print len(pieces)," pieces total"
    
    piece_index = dict() # key = hash of piece. Value = count of source shapes
    for iPiece in range(len(pieces)):
        piece_index[pieces[iPiece].hashCode()] = iPiece
    
    piece_src_counts = [0]*len(pieces)
    
    for list_pieces in map:
        for piece in list_pieces:
            hash = piece.hashCode()
            iPiece = piece_index[hash]
            piece_src_counts[iPiece] += 1
    
    keepers = []
    all_danglers = [] # debug
                
    #add all biggest dangling pieces
    for list_pieces in map:
        danglers = [pieces[piece_index[piece.hashCode()]] for piece in list_pieces if piece_src_counts[piece_index[piece.hashCode()]] == 1]
        all_danglers.extend(danglers)
        largest = shapeOfMaxSize(danglers)
        if largest is not None:
            keepers.append(largest)

    keepers_2 = []
    #add all intersection pieces that touch danglers
    for iPiece in range(len(pieces)):
        if piece_src_counts[iPiece] > 1:
            cnt_touch = 0
            for piece2 in keepers:
                if ShapeMerge.findSharedElements(piece2, pieces[iPiece], lambda(sh): sh.Faces): #FIXME: not faces!
                    cnt_touch += 1
            if cnt_touch > 1:
                keepers_2.append(pieces[iPiece])
    
    #merge, and we are done!
    print len(keepers+keepers_2)," pieces to keep"
    return ShapeMerge.mergeShapes(keepers+keepers_2)
