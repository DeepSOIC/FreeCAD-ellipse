from Show.SceneDetail import SceneDetail

class VProperty(SceneDetail):
    """viewprovider property interface for tempovis"""
    
    class_id = 'SDProperty'
    affects_persistence = True
    propname = ''
    objname = ''
    
    def __init__(self, object, propname, val = None):
        self.objname = object.Name
        self.propname = propname
        self.doc = object.Document
        self.key = self.objname + '.' + self.propname
        self.data = val
        
    def scene_value(self):
        return getattr(self.doc.getObject(self.objname).ViewObject, self.propname)
    
    def apply_data(self, val):
        setattr(self.doc.getObject(self.objname).ViewObject, self.propname, val)
