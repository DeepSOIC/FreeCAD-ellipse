
class SceneDetail(object):
    """SceneDetail class: abstract class for tempovis scene save/restore plug-in. An implementation must provide:
    * data storage (as data attribute of the object)
    * constructor (preferably, with value for stored data as optional argument)
    * methods to apply values to actual scene (apply_data), 
    * ...and to read out the state of the detail in the actual scene (scene_value)
    * keying, for identifying two detail instances that affect the exact same thing
    * class_id string, which is required for keying
    * copying
    * info on if the modification affects the project file, and should be undone temporarily for file writing.
    """
    class_id = ''
    
    data = None
    doc = None
 
    def set_doc(self, doc):
        self.doc = doc
        
 # <interface>
    key = None #a string or something alike to use to store/find the entry in TempoVis. For example, a string "{object_name}.{property_name}". 
    affects_persistence = False #True indicate that the changes will be recorded if the doc is saved, and that this detail should be restored for saving
        
    def scene_value(self):
        """scene_value(): returns the value from the scene"""
        raise NotImplementedError()
    
    def apply_data(self, val):
        """apply a value to scene"""
        raise NotImplementedError()
  # </interface>
  
  # <utility>
    @property
    def full_key(self):
        return (self.class_id, self.key)
    
