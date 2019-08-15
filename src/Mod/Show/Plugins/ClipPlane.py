from Show.TVPlugin import TVPlugin

import FreeCAD as App

class ClipPlane(TVPlugin):
    class_id = 'TVPClipPlane'
    propname = ''
    objname = ''
    
    def __init__(self, object, enable = None, placement = None, offset = 0.0):
        self.objname = object.Name
        self.doc = object.Document
        self.key = self.objname 
        if enable is not None:
            self.data = self.val(enable, placement, offset)
        
    def scene_value(self):
        vp = self.doc.getObject(self.objname).ViewObject
        cp = getClipPlaneNode(vp, make_if_missing= False)
        if cp is None:
            return self.val(False)
        else:
            enable = cp.on.getValue()
            pln = cp.plane
            D = pln.getDistanceFromOrigin()
            normal = tuple(pln.getNormal())
            return enable, (normal, D)
    
    def apply_data(self, val):
        enable, pldef = val
        vp = self.doc.getObject(self.objname).ViewObject
        cp = getClipPlaneNode(vp, make_if_missing= True if enable else False)
        if cp is None and not enable:
            return
        if enable:
            from pivy import coin
            v, d = pldef
            cp.plane.setValue(coin.SbPlane(coin.SbVec3f(*v), d))
        cp.on.setValue(enable)
    
    def val(self, enable, placement = None, offset = 0.0):
        """val(enable, placement = None, offset = 0.0): constructs a value from convenient 
        parameters. Placement is in global CS. The cutting will be by XY plane of Placement; 
        the stuff in negative Z is visible and the stuff in positive Z is invisible."""
        
        pldef = None
        if enable:
            obj = self.doc.getObject(self.objname)
            plm_cs = obj.getGlobalPlacement().multiply(obj.Placement.inverse()) # placement of CS the object is in
            plm_plane = plm_cs.inverse().multiply(placement)
            pldef = placement2plane(plm_plane, offset)
        return (enable, pldef if enable else None)

def getClipPlaneNode(viewprovider, make_if_missing = True):
    from pivy import coin
    sa = coin.SoSearchAction()
    sa.setType(coin.SoClipPlane.getClassTypeId())
    sa.setName('TVClipPlane')
    sa.traverse(viewprovider.RootNode)
    if sa.isFound() and sa.getPath().getLength() == 1:
        return sa.getPath().getTail()
    elif not sa.isFound():
        if not make_if_missing:
            return None
        clipplane = coin.SoClipPlane()
        viewprovider.RootNode.insertChild(clipplane, 0)
        clipplane.setName('TVClipPlane')
        clipplane.on.setValue(False) #make sure the plane is not activated by default
        return clipplane

def placement2plane(placement, offset):
    """returns tuple (normal, D) for making coin plane."""
    normal = placement.Rotation.multVec(App.Vector(0,0,-1))
    D = -(placement.Base * normal + offset)
    return tuple(normal), D

def clipPlane(obj, enable, placement = None, offset = 0, tv = None):
    if tv is None:
        from Show.TempoVis import TempoVis
        tv = TempoVis(obj.Document)
    tv.modify(ClipPlane(obj, enable, placement, offset))
    return tv
