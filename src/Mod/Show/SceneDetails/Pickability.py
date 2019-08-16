from Show.SceneDetail import SceneDetail

class Pickability(SceneDetail):
    class_id = 'SDPickability'
    propname = ''
    objname = ''
    
    def __init__(self, object, pickstyle = None):
        self.objname = object.Name
        self.doc = object.Document
        self.key = self.objname 
        if pickstyle is not None:
            self.data = pickstyle
        
    def scene_value(self):
        return getPickStyle(self.doc.getObject(self.objname).ViewObject)
    
    def apply_data(self, val):
        setPickStyle(self.doc.getObject(self.objname).ViewObject, val)

PS_REGULAR = 0
PS_BOUNDBOX = 1
PS_UNPICKABLE = 2

def getPickStyleNode(viewprovider, make_if_missing = True):
    from pivy import coin
    sa = coin.SoSearchAction()
    sa.setType(coin.SoPickStyle.getClassTypeId())
    sa.traverse(viewprovider.RootNode)
    if sa.isFound() and sa.getPath().getLength() == 1:
        return sa.getPath().getTail()
    else:
        if not make_if_missing:
            return None
        pick_style = coin.SoPickStyle()
        pick_style.style.setValue(coin.SoPickStyle.SHAPE)
        viewprovider.RootNode.insertChild(pick_style, 0)
        return pick_style
            

def getPickStyle(viewprovider):
    ps = getPickStyleNode(viewprovider, make_if_missing= False)
    if ps is not None:
        return ps.style.getValue()
    else:
        return PS_REGULAR

def setPickStyle(viewprovider, pickstyle):
    ps = getPickStyleNode(viewprovider, make_if_missing= pickstyle != 0) #coin.SoPickStyle.SHAPE
    if ps is not None:
        return ps.style.setValue(pickstyle)

