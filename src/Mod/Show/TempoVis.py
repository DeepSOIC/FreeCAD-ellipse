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


from .DepGraphTools import getAllDependencies, getAllDependent
from .Utils import is3DObject

from . import Containers
Container = Containers.Container

from . import TVStack

import FreeCAD as App
if App.GuiUp:
    import FreeCADGui as Gui
warn = lambda msg: App.Console.PrintWarning(msg + "\n")

from copy import copy

S_EMPTY = 0 # TV is initialized, but no changes were done through it
S_ACTIVE = 1 # TV has something to be undone 
S_RESTORED = 2 # TV has been restored
S_INTERNAL = 3 # TV instance is being used by another TV instance as a redo data storage

class MAINSTACK(object):
    """it's just a default value definition for TV constructor"""
    pass

class TempoVis(object):
    '''TempoVis - helper object to save visibilities of objects before doing
    some GUI editing, hiding or showing relevant stuff during edit, and
    then restoring all visibilities after editing.

    Constructors:
    TempoVis(document, stack = MAINSTACK): creates a new TempoVis. 
    
    document: required. Objects not belonging to the document can't be modified via TempoVis.
    
    stack: optional. Which stack to insert this new TV into. Can be:
    a TVStack instance (then, the new TV is added to the top of the stack), 
    MAINSTACK special value (a global stack for the document will be used), or 
    None (then, the TV is not in any stack, and can be manually instertd into one if desired).'''

    document = None
    stack = None # reference to stack this TV is in

    data = None # dict. key = ("class_id","key"), value = instance of plugin
    data_redo = None
    
    sketch_clipplane_on = False #True if some clipping planes are active
        
    links_are_lost = False # set to true after restore from JSON. Indicates to attempt to use ActiveDocument/ActiveViewer instead.
    
    state = S_EMPTY
    

    def _init_attrs(self):
        """initialize member variables to empty values (needed because we can't use mutable initial values when initializing member variables in class definition)"""
        self.data = {}
        self.data_redo = {}
        
  #<core interface>
    def __init__(self, document, stack = MAINSTACK):
        self._init_attrs()
        self.document = document
        
        if stack is MAINSTACK:
            stack = TVStack.main_stack(document)
        
        if stack is None:
            pass
        else:
            stack.insert(self)
    
    def __del__(self):
        if self.state == S_ACTIVE:
            self.restore(save_redo= False, ultimate= True)
    
    def has(self, detail):
        """has(self, detail): returns True if this TV has this detail value saved.
        example: tv.has(VProperty(obj, "Visibility"))"""
        return detail.full_key in self.data
    
    def stored_val(self, detail):
        """stored_val(self, detail): returns value of detail remembered by this TV. If not, raises KeyError."""
        return self.data[detail.full_key].data

    def save(self, detail):
        """save(detail):saves the scene detail to be restored. The detail is saved only once; repeated calls are ignored."""
        self._change()
        if not detail.full_key in self.data:
            va = self._value_after(detail)
            if va is None:
                self.data[detail.full_key] = copy(detail)
                self.data[detail.full_key].data = detail.scene_value()
            else:
                self.data[detail.full_key] = copy(va[1])

    def modify(self, detail):
        """modify(detail): modifies scene detail through this TV. 
        The value is provided as an instance of plugin class.
        
        Example: tv.modify(VProperty(obj, "Visibility", True))"""
        self._change()
        
        self.save(detail)
        va = self._value_after(detail)
        if va is not None:
            tv1, detail1 = va
            detail1.data = detail.data
        else:
            detail.apply_data(detail.data)
    
    def restoreDetail(self, detail, save_redo = False, ultimate = False):
        if not self.has(detail):
            return
        p = self.data[detail.full_key]
        va = self._value_after(detail)
        if save_redo:
            self.data_redo[detail.full_key] = copy(detail)
            self.data_redo[detail.full_key].data = detail.scene_value() if va is None else va[1].data
        if va is None:
            # no other TV has changed this detail later, apply to the scene
            detail.apply_data(detail.data)
        else:
            #modify saved detail of higher TV
            tv1, detail1 = va
            detail1.data = detail.data
        if ultimate:
            self.forgetDetail(detail)
    
    def forgetDetail(self, detail):
        self.data.pop(detail.full_key)
    
    def forget(self):
        self.state = S_EMPTY
        self.data = {}
        if self.is_in_stack:
            self.stack.withdraw(self)

    def restore(self, save_redo = False, ultimate = True):
        """restore(save_redo = False, ultimate = True): undoes all changes done through this tempovis / restores saved scene details.
        save_redo: if True, the restring can be undone by redo method.
        ultimate: if true, the saved values are cleaned out, and the TV is withdrawn from the stack. If false, the TV will still remember stuff, and resore can be called again."""
        
        if self.state != S_INTERNAL and ultimate:
            self.state = S_RESTORED
        
        for key, detail in self.data.items():
            va = self._value_after(detail)
            if save_redo:
                self.data_redo[detail.full_key] = copy(detail)
                self.data_redo[detail.full_key].data = detail.scene_value() if va is None else va[1].data
            if va is None:
                # no other TV has changed this detail later, apply to the scene
                detail.apply_data(detail.data)
            else:
                #modify saved detail of higher TV
                tv1, detail1 = va
                detail1.data = detail.data
        if ultimate:
            self.data = {}
            if self.is_in_stack:
                self.stack.withdraw(self)
    
  #</core interface>
  
  #<stack interface>
    def _inserted(self, stack, index):
        self.stack = stack
    def _withdrawn(self, stack, index):
        self.stack = None
    @property
    def is_in_stack(self):
        return self.stack is not None
  #</stack interface>

  #<convenience functions>        
    def orig_val(self, detail):
        """orig_val(self, detail): returns value of detail before any TV was applied."""
        va = self.stack.value_after(None, detail)
        if va is None:
            detail.document = self.document
            return detail.scene_value()
        else:
            return va[1].data
    
    def _value_after(self, detail):
        if self.is_in_stack:
            return self.stack.value_after(self, detail)
        else:
            return None

    def modifyVPProperty(self, doc_obj_or_list, prop_name, new_value):
        '''modifyVPProperty(self, doc_obj_or_list, prop_name, new_value): modifies
        prop_name property of ViewProvider of doc_obj_or_list, and remembers
        original value of the property. Original values will be restored upon
        TempoVis deletion, or call to restore().'''
        
        if self.state == S_RESTORED:
            warn("Attempting to use a TV that has been restored. There must be a problem with code.")
            return

        if App.GuiUp:
            if not hasattr(doc_obj_or_list, '__iter__'):
                doc_obj_or_list = [doc_obj_or_list]
            for doc_obj in doc_obj_or_list:
                if not is3DObject(doc_obj):
                    continue
                if not hasattr(doc_obj.ViewObject, prop_name):
                    warn("TempoVis: object {obj} has no attribute {attr}. Skipped."
                                             .format(obj= doc_obj.Name, attr= prop_name))
                    continue # silently ignore if object doesn't have the property...

                if doc_obj.Document is not self.document:  #ignore objects from other documents
                    raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
                from .Plugins.VProperty import VProperty
                self.modify(VProperty(doc_obj, prop_name, new_value))

    def show(self, doc_obj_or_list):
        '''show(doc_obj_or_list): shows objects (sets their Visibility to True). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, 'Visibility', True)

    def hide(self, doc_obj_or_list):
        '''hide(doc_obj_or_list): hides objects (sets their Visibility to False). doc_obj_or_list can be a document object, or a list of document objects'''
        self.modifyVPProperty(doc_obj_or_list, 'Visibility', False)

    def get_all_dependent(self, doc_obj):
        '''get_all_dependent(doc_obj): gets all objects that depend on doc_obj. Containers of the object are excluded from the list.'''
        cnt_chain = Containers.ContainerChain(doc_obj)
        return [o for o in getAllDependent(doc_obj) if not o in cnt_chain]

    def hide_all_dependent(self, doc_obj):
        '''hide_all_dependent(doc_obj): hides all objects that depend on doc_obj. Groups, Parts and Bodies are not hidden by this.'''
        self.hide( self.get_all_dependent(doc_obj) )

    def show_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): shows all objects that depend on doc_obj. This method is probably useless.'''
        self.show( getAllDependent(doc_obj) )

    def restore_all_dependent(self, doc_obj):
        '''show_all_dependent(doc_obj): restores original visibilities of all dependent objects.'''
        self.restoreVPProperty( getAllDependent(doc_obj), 'Visibility' )

    def hide_all_dependencies(self, doc_obj):
        '''hide_all_dependencies(doc_obj): hides all objects that doc_obj depends on (directly and indirectly).'''
        self.hide( getAllDependencies(doc_obj) )

    def show_all_dependencies(self, doc_obj):
        '''show_all_dependencies(doc_obj): shows all objects that doc_obj depends on (directly and indirectly). This method is probably useless.'''
        self.show( getAllDependencies(doc_obj) )

    def saveCamera(self, vw = None):
        self._change()
        from .Plugins.Camera import Camera
        self.save(Camera(self.document))
        
    def restoreCamera(self, save_redo = False, ultimate = False):
        from .Plugins.Camera import Camera
        dt = Camera(self.document)
        self.restoreDetail(dt,save_redo,ultimate)

    def setUnpickable(self, doc_obj_or_list, actual_pick_style = 2): #2 is coin.SoPickStyle.UNPICKABLE
        '''setUnpickable(doc_obj_or_list, actual_pick_style = 2): sets object unpickable (transparent to clicks).
        doc_obj_or_list: object or list of objects to alter (App)
        actual_pick_style: optional parameter, specifying the actual pick style: 
        0 = regular, 1 = bounding box, 2 (default) = unpickable.
        
        Implementation detail: uses SoPickStyle node. If viewprovider already has a node 
        of this type as direct child, one is used. Otherwise, new one is created and 
        inserted as the very first node, and remains there even after restore()/deleting 
        tempovis. '''
        
        from .Plugins.Pickability import Pickability
        
        if not hasattr(doc_obj_or_list, '__iter__'):
            doc_obj_or_list = [doc_obj_or_list]
        for doc_obj in doc_obj_or_list:
            if not is3DObject(doc_obj):
                continue
            if doc_obj.Document is not self.document:  #ignore objects from other documents
                raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
            dt = Pickability(doc_obj, actual_pick_style)
            self.modify(dt)

    def clipPlane(self, doc_obj_or_list, enable, placement, offset = 0.02): 
        '''clipPlane(doc_obj_or_list, enable, placement, offset): slices off the object with a clipping plane.
        doc_obj_or_list: object or list of objects to alter (App)
        enable: True if you want clipping, False if you want to remove clipping: 
        placement: XY plane of local coordinates of the placement is the clipping plane. The placement must be in document's global coordinate system.
        offset: shifts the plane. Positive offset reveals more of the object.
        
        Implementation detail: uses SoClipPlane node. If viewprovider already has a node 
        of this type as direct child, one is used. Otherwise, new one is created and 
        inserted as the very first node. The node is left, but disabled when tempovis is restoring.'''
        
        from .Plugins.ClipPlane import ClipPlane
        
        if not hasattr(doc_obj_or_list, '__iter__'):
            doc_obj_or_list = [doc_obj_or_list]
        for doc_obj in doc_obj_or_list:
            if not is3DObject(doc_obj):
                continue
            if doc_obj.Document is not self.document:  #ignore objects from other documents
                raise ValueError("Document object to be modified does not belong to document TempoVis was made for.")
            dt = ClipPlane(doc_obj, enable, placement, offset)
            self.modify(dt)
                    
    @staticmethod
    def allVisibleObjects(aroundObject):
        """allVisibleObjects(aroundObject): returns list of objects that have to be toggled invisible for only aroundObject to remain. 
        If a whole container can be made invisible, it is returned, instead of its child objects."""
        
        chain = Containers.VisGroupChain(aroundObject)
        result = []
        for i in range(len(chain)):
            cnt = chain[i]
            cnt_next = chain[i+1] if i+1 < len(chain) else aroundObject
            for obj in Container(cnt).getVisGroupChildren():
                if not is3DObject(obj):
                    continue
                if obj is not cnt_next:
                    if obj.ViewObject.Visibility:
                        result.append(obj)
        return result
        
    def sketchClipPlane(self, sketch, enable = None):
        """sketchClipPlane(sketch, enable = None): Clips all objects by plane of sketch. 
        If enable argument is omitted, calling the routine repeatedly will toggle clipping plane."""
        
        if enable is None:
            enable = not self.sketch_clipplane_on
            self.sketch_clipplane_on = enable
        self.clipPlane(self.allVisibleObjects(sketch), enable, sketch.getGlobalPlacement(), 0.02)
  #</convenience functions>

  #<internals>
    def _change(self):
        """to be called whenever anything is done that is to be restored later."""
        if self.state == S_EMPTY:
            self.state = S_ACTIVE
        if self.state == S_RESTORED:
            warn("Attempting to use a TV that has been restored. There must be a problem with code.")
        self.tv_redo = None    

    def __getstate__(self):
        raise NotImplementedError()
        
    def __setstate__(self, state):
        self._init_attrs()
        raise NotImplementedError()

    
        

