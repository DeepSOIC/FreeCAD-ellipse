
class TVPlugin(object):
    """TVPlugin class: base class for tempovis data entries"""
    data = None
    doc = None
 
    def set_doc(self, doc):
        self.doc = doc
        
 # <interface>
    key = None #a string or something alike to use to store/find the entry in TempoVis. For example, a string "{object_name}.{property_name}". 
            
    def scene_value(self):
        """scene_value(): returns the value from the scene"""
        raise NotImplementedError()
    
    def apply_data(self, val):
        """apply a value to scene"""
        raise NotImplementedError()
  # </interface>
  
  # <utility>
    def set(self, val):
        return apply_data(self, val)
        
    @property
    def full_key(self):
        return (self.class_id, self.key)
    
