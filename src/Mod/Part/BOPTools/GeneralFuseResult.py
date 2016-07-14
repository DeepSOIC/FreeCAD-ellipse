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

__title__="BOPTools.GeneralFuseResult module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecadweb.org"
__doc__ = "Implementation of GeneralFuseResult class, that parses return of generalFuse."

import Part
from .Utils import HashableShape, HashableShape_Deep, FrozenClass


class GeneralFuseResult(FrozenClass):
    """class GeneralFuseResult: helper object for obtaining info from results of 
    Part.Shape.generalFuse() method. 
    
Usage:
def myCustomFusionRoutine(list_of_shapes):
    generalFuse_return = list_of_shapes[0].generalFuse(list_of_shapes[1:])
    ao = GeneralFuseResult(list_of_shapes, generalFuse_return)
    ... (use attributes and methods of ao) ..."""
    
    def __define_attributes(self):
        self.gfa_return = None #stores the data returned by generalFuse, supplied to class constructor

        self.pieces = None #pieces that resulted from intersetion routine. List of shapes (non-decorated).
        self._piece_to_index = {} # key = decorated shape. Value = index (int) into self.pieces

        self.source_shapes = [] # list of shapes that was supplied to generalFuse (plus the self-shape). List of shapes (non-decorated)
        self._source_to_index = {} # key = decorated shape. Value = index (int) into self.source_shapes

        self._pieces_of_source = [] #list of pieces (indexes) generated from a source shape, by index of source shape. List of lists of ints.
        self._sources_of_piece = [] #list of source shapes (indexes) the piece came from, by index of piece. List of lists of ints.
        
        self._element_to_source = {} #dictionary for finding, which source shapes did an element of pieces come from. key = HashableShape (element). Value = set of ints 
        
        self._freeze()
    
    def __init__(self, source_shapes, generalFuse_return):
        self.__define_attributes()
        
        self.gfa_return = generalFuse_return
        self.source_shapes = source_shapes
        self.parse()

    def parse(self):
        #save things to be parsed and wipe out all other data
        gfa_return = self.gfa_return
        source_shapes = self.source_shapes
        self.__define_attributes()
        self.gfa_return = gfa_return
        self.source_shapes = source_shapes
        
        # fill in data structures.
        compound, map = self.gfa_return
        self.pieces = compound.childShapes()
        
        for iPiece in range(len(self.pieces)):
            ha_piece = HashableShape(self.pieces[iPiece])
            if not ha_piece in self._piece_to_index:
                self._piece_to_index[ha_piece] = iPiece
            else:
                raise ValueError("GeneralFuseAnalyzer.parse: duplicate piece shape detected.")
            
        
        #first, let's collect all source shape types, and warn about known problematic cases, and fix some of them
        #known problems:
        # for list-like source shapes, pieces are not split, and map gets an empty entry
        types = set()
        for source_shape in self.source_shapes:
            types.add(source_shape.ShapeType)
        if "Compound" in types:
            import FreeCAD as App
            App.Console.PrintWarning("GeneralFuseAnalyzer.parse: there are compounds among source shapes. This is known to cause problems.\n")
        listy_types = set(["Wire","Shell","CompSolid","Compound"])
        nonlisty_types = set(["Vertex","Edge","Face","Solid"])
        if types.issubset(listy_types):
            # pieces directly match the structure of source list. Recover map.
            if len(map[0]) == 0: #but recover only if needed, to not mess up data prepared by GeneralFuseReturnBuilder
                map = [[piece] for piece in self.pieces]
            
        elif types.issubset(nonlisty_types):
            #all right, nothing to do
            pass
        else:
            # seems like it's impossible to recover the map
            raise TypeError("Mix of shapes was supplied for which generalFuse does not return enough information ({types}).".format(types= ", ".join(types)))
            
        
        for iSource in range(len(self.source_shapes)):
            ha_source = HashableShape(self.source_shapes[iSource])
            if not ha_source in self._source_to_index:
                self._source_to_index[ha_source] = iSource
            else:
                raise ValueError("GeneralFuseAnalyzer.parse: duplicate source shape detected.")
        
        self._pieces_of_source = [[] for i in range(len(self.source_shapes))]
        self._sources_of_piece = [[] for i in range(len(self.pieces))]
        assert(len(map) == len(self.source_shapes))
        for iSource in range(len(self.source_shapes)):
            list_pieces = map[iSource]
            for piece in list_pieces:
                iPiece = self.indexOfPiece(piece)
                self._sources_of_piece[iPiece].append(iSource)
                self._pieces_of_source[iSource].append(iPiece)
    
    def parse_elements(self):
        """Fills element-to-source map. Potentially slow, so separated from general parse."""
        if len(self._element_to_source)>0:
            return #already parsed.
            
        for iPiece in range(len(self.pieces)):
            piece = self.pieces[iPiece]
            for element in piece.Vertexes + piece.Edges + piece.Faces:
                el_h = HashableShape(element)
                if el_h in self._element_to_source:
                    self._element_to_source[el_h].update(set(self._sources_of_piece[iPiece]))
                else:
                    self._element_to_source[el_h] = set(self._sources_of_piece[iPiece])
    
    def indexOfPiece(self, piece_shape):
        "indexOfPiece(self, piece_shape): returns index of piece_shape in list of pieces"
        return self._piece_to_index[HashableShape(piece_shape)]
    def indexOfSource(self, source_shape):
        "indexOfSource(self, source_shape): returns index of source_shape in list of arguments"
        return self._source_to_index[HashableShape(source_shape)]
    
    def piecesFromSource(self, source_shape):
        "piecesFromSource(self, source_shape): returns list of pieces (shapes) that came from given source shape."
        ilist = self._pieces_of_source[self.indexOfSource(source_shape)]
        return [self.pieces[i] for i in ilist]
    
    def sourcesOfPiece(self, piece_shape):
        "sourcesOfPiece(self, piece_shape): returns list of source shapes given piece is part of."
        ilist = self._sources_of_piece[self.indexOfPiece(piece_shape)]
        return [self.source_shapes[i] for i in ilist]
        
    def largestOverlapCount(self):
        return max([len(ilist) for ilist in self._sources_of_piece])
    
    def splitWiresShells(self):
        """splitWiresShells(self): splits wires and shells as cut by intersections. Note: this routine is heavy and fragile."""
        from . import ShapeMerge
        self.parse_elements()
        new_data = GeneralFuseReturnBuilder(self.source_shapes)
        for iPiece in range(len(self.pieces)):
            piece = self.pieces[iPiece]
            if piece.ShapeType == "Wire":
                bit_extractor = lambda(sh): sh.Edges
                joint_extractor = lambda(sh): sh.Vertexes
            elif piece.ShapeType == "Shell":
                bit_extractor = lambda(sh): sh.Faces
                joint_extractor = lambda(sh): sh.Edges
            else:
                #there is no need to split the piece
                new_data.addPiece(self.pieces[iPiece], self._sources_of_piece[iPiece])
                continue
                
            # for each joint, test if all bits it's connected to are from same number of sources. If not, this is a joint for splitting
            splits = []
            for joint in joint_extractor(piece):
                joint_overlap_count = len(self._element_to_source[HashableShape(joint)])
                if joint_overlap_count > 1:
                    # find elements in pieces that are connected to joint
                    for bit in bit_extractor(self.gfa_return[0]):
                        for joint_bit in joint_extractor(bit):
                            if joint_bit.isSame(joint):
                                #bit is connected to joint!
                                bit_overlap_count = len(self._element_to_source[HashableShape(bit)])
                                assert(bit_overlap_count <= joint_overlap_count)
                                if bit_overlap_count < joint_overlap_count:
                                    if len(splits) == 0 or splits[-1] is not joint:
                                        splits.append(joint)
            print "piece ", iPiece,": number of splits = ",len(splits)
            if len(splits)==0:
                #piece was not split - no split points found
                new_data.addPiece(self.pieces[iPiece], self._sources_of_piece[iPiece])
                continue
                
            new_pieces = ShapeMerge.mergeShapes(bit_extractor(piece), split_connections= splits).childShapes()
            if len(new_pieces) == 1:
                #piece was not split (split points found, but the piece remained in one piece).
                new_data.addPiece(self.pieces[iPiece], self._sources_of_piece[iPiece])
                continue

            for new_piece in new_pieces:
                new_data.addPiece(new_piece, self._sources_of_piece[iPiece])
        
        if len(new_data.pieces) > len(self.pieces):
            self.gfa_return = new_data.getGFReturn()
            self.parse()
        else:
            print "Nothing was split"
    
            
class GeneralFuseReturnBuilder(FrozenClass):
    "GeneralFuseReturnBuilder: utility class used by splitWiresShells to build fake return of generalFuse, for re-parsing."
    def __define_attributes(self):
        self.pieces = []
        self._piece_to_index = {} #deep hash
        
        self._pieces_from_source = [] #list of list of ints
        self.source_shapes = []
        
        self._freeze()
        
    def __init__(self, source_shapes):
        self.__define_attributes()
        self.source_shapes = source_shapes
        self._pieces_from_source = [[] for i in range(len(source_shapes))]
    
    def addPiece(self, piece_shape, source_shape_index_list):
        hash = HashableShape_Deep(piece_shape)
        i_piece_existing = self._piece_to_index.get(hash)
        if i_piece_existing is None:
            #adding
            self.pieces.append(piece_shape)
            i_piece_existing = len(self.pieces)-1
            self._piece_to_index[hash] = i_piece_existing
        else:
            #re-adding
            pass
        for iSource in source_shape_index_list:
            if not i_piece_existing in self._pieces_from_source[iSource]:
                self._pieces_from_source[iSource].append(i_piece_existing)
    
    def getGFReturn(self):
        return (Part.Compound(self.pieces), [[self.pieces[iPiece] for iPiece in ilist] for ilist in self._pieces_from_source])