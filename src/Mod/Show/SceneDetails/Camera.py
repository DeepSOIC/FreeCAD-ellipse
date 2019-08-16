from Show.SceneDetail import SceneDetail

import FreeCADGui

class Camera(SceneDetail):
    class_id = 'SDCamera'
    
    def __init__(self, doc):
        self.doc = doc
        self.key = 'the_cam'
            
    def _viewer(self):
        gdoc = FreeCADGui.getDocument(self.doc.Name)
        v = gdoc.activeView()
        if not hasattr(v, 'getCamera'):
            v = gdoc.mdiViewsOfType('Gui::View3DInventor')[0]
        return v

    def scene_value(self):
        return self._viewer().getCamera()
    
    def apply_data(self, val):
        self._viewer().setCamera(val)

