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
from .Utils import HashableShape, FrozenClass


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

        for iSource in range(len(self.source_shapes)):
            ha_source = HashableShape(self.source_shapes[iSource])
            if ha_source.Shape.ShapeType == "Compound":
                import FreeCAD as App
                App.Console.PrintWarning("GeneralFuseAnalyzer.parse: there are compounds among source shapes. This is known to cause problems.\n")
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