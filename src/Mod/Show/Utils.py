def typeString(cls):
    return cls.__module__ + ':' + cls.__name__

def classFromTypeString(tstr):
    mod, cls = tstr.split(':')
    import importlib
    importlib.import_module(mod)
    return getattr(mod, cls)
    
def is3DObject(obj):
    """is3DObject(obj): tests if the object has some 3d geometry. 
    TempoVis is made only for objects in 3d view, so all objects that don't pass this check are ignored by TempoVis."""
    
    # See "Gui Problem Sketcher and TechDraw" https://forum.freecadweb.org/viewtopic.php?f=3&t=22797

    # observation: all viewproviders have transform node, then a switch node. If that switch node contains something, the object has something in 3d view.
    try:
        viewprovider = obj.ViewObject
        from pivy import coin
        sa = coin.SoSearchAction()
        sa.setType(coin.SoSwitch.getClassTypeId())
        sa.traverse(viewprovider.RootNode)
        if not sa.isFound(): #shouldn't happen...
            return False 
        n = sa.getPath().getTail().getNumChildren()
        return n > 0
    except Exception as err:
        App.Console.PrintWarning(u"Show.TempoVis.isIn3DView error: {err}".format(err= str(err)))
        return True #assume.
