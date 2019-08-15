
from . import TVStack
import FreeCAD

class TVObserver(object):
    def __init__(self):
        FreeCAD.addDocumentObserver(self)
    
    def stop(self):
        FreeCAD.removeDocumentObserver(self)
    
    def slotStartSaveDocument(self, doc, filepath):
        TVStack._slotStartSaveDocument(doc)
    
    def slotFinishSaveDocument(self, doc, filepath):
        TVStack._slotFinishSaveDocument(doc)
        
    def slotDeletedDocument(self, doc):
        from . import TVStack
        TVStack._slotDeletedDocument(doc)

#handle module reload
if 'observer_singleton' in vars():
    observer_singleton.stop()

observer_singleton = TVObserver()
