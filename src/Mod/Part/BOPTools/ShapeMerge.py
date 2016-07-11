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

__doc__ = "JoinFeatures helper low-level functionality - functions for merging shapes obtained from generalFuse."
import Part

def findSharedElements(shape1, shape2, element_extractor):
    shared_elements = []
    for element1 in element_extractor(shape1):
        for element2 in element_extractor(shape2):
            if element1.isSame(element2):
                shared_elements.append(element1)
    return shared_elements

def splitIntoGroupsBySharing(list_of_shapes, element_extractor):
    """splitIntoGroupsBySharing(list_of_shapes, element_type): find, which shapes in list_of_shapes 
    are connected into groups by sharing elements. 
    
    element_extractor: function that takes shape as input, and returns list of shapes.
    
    return: list of lists of shapes. Top-level list is list of groups; bottom level lists 
    enumerate shapes of a group."""
    
    
    groups = [] #list of tuples (shapes,elements).
    
    # add shapes to the list of groups, one by one. If not connected to existing groups, 
    # new group is created. If connected, shape is added to groups, and the groups are joined.
    for shape in list_of_shapes:
        #FIXME: relying on hash code is weak. There is a small chance of hash coincidence.
        shape_elements = set([element.hashCode() for element in element_extractor(shape)])
        #search if shape is connected to any groups
        connected_to = []
        for iGroup in range(len(groups)):
            connected = False
            for element in shape_elements:
                if element in groups[iGroup][1]:
                    connected_to.append(iGroup)
                    connected = True
                    break
        # test if we need to join groups
        if len(connected_to)>1:
            #shape bridges a gap between some groups. Join them into one.
            #rebuilding list of groups. First, add the new "supergroup", then add the rest
            groups_new = []
            
            supergroup = (list(),set())
            for iGroup in connected_to:
                supergroup[0].extend( groups[iGroup][0] )# merge lists of shapes
                supergroup[1].update( groups[iGroup][1] )# merge lists of elements
            groups_new.append(supergroup)
            
            for iGroup in range(len(groups)):
                if not iGroup in connected_to: #fixme: inefficient!
                    groups_new.append(groups[iGroup])
            groups = groups_new
            connected_to = [0]
            
        # add shape to the group it is connected to (if to many, the groups should have been unified by the above code snippet)
        if len(connected_to) > 0:
            iGroup = connected_to[0]
            groups[iGroup][0].append(shape)
            groups[iGroup][1].update( shape_elements )
        else:
            newgroup = ([shape], shape_elements)
            groups.append(newgroup)
    
    # done. Discard unnecessary data and return result.
    return [shapes for shapes,elements in groups]

def mergeSolids(list_of_solids_compsolids, flag_single = False):
    """mergeSolids(list_of_solids, flag_single = False): merges touching solids that share 
    faces. If flag_single is True, it is assumed that all solids touch, and output is a 
    single solid. If flag_single is False, the output is a compound containing all 
    resulting solids.
    
    Note. CompSolids are treated as lists of solids - i.e., merged into solids."""

    solids = []
    for sh in list_of_solids_compsolids:
        solids.extend(sh.Solids)
    if flag_single:
        return Part.makeSolid(Part.CompSolid(solids))
    else:
        if len(solids)==0:
            return Part.Compound([])
        groups = splitIntoGroupsBySharing(solids, lambda(sh): sh.Faces)
        merged_solids = [Part.makeSolid(Part.CompSolid(group)) for group in groups]
        return Part.makeCompound(merged_solids)

def mergeShells(list_of_faces_shells):
    faces = []
    for sh in list_of_faces_shells:
        faces.extend(sh.Faces)
    return Part.makeShell(faces)
    
def mergeWires(list_of_edges_wires, flag_single = False):
    edges = []
    for sh in list_of_edges_wires:
        edges.extend(sh.Edges)
    if flag_single:
        return Part.Wire(edges)
    else:
        groups = splitIntoGroupsBySharing(edges, lambda(sh): sh.Vertexes)
        return Part.makeCompound([Part.Wire(group) for group in groups])
        
def mergeVertices(list_of_vertices):
    return Part.makeCompound(list_of_vertices)

def mergeShapes(list_of_shapes):
    if len(list_of_shapes)==0:
        return Part.Compound([])
    dim = dimensionOfShapes(list_of_shapes)
    if dim == 0:
        return mergeVertices(list_of_shapes)
    elif dim == 1:
        return mergeWires(list_of_shapes)
    elif dim == 2:
        return mergeShells(list_of_shapes)
    elif dim == 3:
        return mergeSolids(list_of_shapes)
    else:
        assert(dim >= 0 and dim <= 3)

def removeDuplicates(list_of_shapes):
    #FIXME: relying on hash only is weak.
    hashes = set()
    new_list = []
    for sh in list_of_shapes:
        hash = sh.hashCode()
        if hash in hashes:
            pass
        else:
            new_list.append(sh)
            hashes.add(hash)
    return new_list
    
def dimensionOfShapes(list_of_shapes):
    """dimensionOfShapes(list_of_shapes): returns dimension (0D, 1D, 2D, or 3D) of shapes 
    in the list. If dimension of shapes varies, TypeError is raised."""
    
    dimensions = [["Vertex"], ["Edge","Wire"], ["Face","Shell"], ["Solid","CompSolid"]]
    dim = -1
    for sh in list_of_shapes:
        sht = sh.ShapeType
        for iDim in range(len(dimensions)):
            if sht in dimensions[iDim]:
                if dim == -1:
                    dim = iDim
                if iDim != dim:
                    raise TypeError("Shapes are of different dimensions ({t1} and {t2}), and cannot be merged or compared.".format(t1= list_of_shapes[0].ShapeType, t2= sht))
    return dim