from . import TempoVis
from . import TVObserver #import to start the observer

import weakref

global_stacks = {} # dict of TVStacks. key = document name.

class TVStack(object):
    stack_index = None # Key = id(tempovis_instance). Value = index into list in tvstack.
    stack = None #list of weakrefs to TV instances
    document = None
    
    _rewind_tv = None

    def __init__(self, document):
        self.document = None
        self.stack_index = {}
        self.stack = []

    def insert(self, tv, index = None):
        if index is None:
            index = len(self.stack)
        ref = weakref.ref(tv, (lambda idtv=id(tv), self=self : self._destruction(idtv)))
        self.stack.insert(index, ref)
        
        self.rebuild_index(index)

        tv._inserted(self, index)
    
    def _destruction(self, idtv):
        index = self.stack_index[idtv]
        ref = stack[index]
        tv = ref()
        if tv is not None:
            if tv.state == TempoVis.S_ACTIVE:
                tv.restore()
                #this call will withdraw the TV from the stack, so we are done.
                return
        #otherwise:
        self.stack.pop(index)
        self.stack_index.pop(idtv)

        self.rebuild_index(index)
        if tv is not None:
            tv._withdrawn(index)
    
    def withdraw_id(self, idtv):
        """withdraw_id(idtv): removes tv by id from the stack"""
        index = self.stack_index[idtv]
        ref = self.stack.pop(index)
        self.stack_index.pop(idtv)

        self.rebuild_index(index)
        
        tv = ref()
        if tv:
            tv._withdrawn(self, index)
        
    def withdraw(self, tv):
        self.withdraw_id(id(tv))
    
    def value_after(self, tv, detail):
        """value_after(tv, detail): returns tuple (tv1, detail), or None. 
        Here, tv1 is the tv that remembers the value, and detail is reference to recorded 
        data in tv1. None is returned, if no TVs in the stack after the provided one have 
        recorded a change to this detail.
 
        tv can be None, then, the function returns the original value of the detail, or 
        None, if the current value matches the original."""
        
        index = self.stack_index[id(tv)] if tv is not None else -1
        for tvref in self.stack[index + 1 : ]:
            tv = tvref()
            if tv.state == TempoVis.S_ACTIVE:
                if tv.has(detail):
                    return (tv, tv.data[detail.full_key])
        return None
        
    def rebuild_index(self, start = 0):
        if start == 0:
            self.stack_index = {}
        for i in range(start, len(self.stack)):
            self.stack_index[id(self.stack[i]())] = i
        
    
    def purge_dead(self):
        """removes dead TV instances from the stack"""
        n = 0
        for i in reversed(range(len(self.stack))):
            if self.stack[i]() is None:
                self.stack.pop(i)
                n += 1
        if n > 0:
            self.rebuild_index()
        return n
    
    def dissolve(self):
        """silently cleans all TVs, so that they won't restore."""
        for ref in self.stack:
            if ref() is not None:
                ref().forget()
    
    def unwindForSaving(self):
        self.rewindAfterSaving() #just in case there was a failed save before.
        
        details = {} #dict of detail original values. Key = detail key; value = detail instance with data representing the original value
        for ref in self.stack:
            tv = ref()
            for key, detail in tv.data.items():
                if not key in details:
                    if detail.affects_persistence:
                        details[detail.full_key] = detail
        
        self._rewind_tv = TempoVis.TempoVis(self.document, None)
        for key, detail in details.items():
            self._rewind_tv.modify(detail)
    
    def rewindAfterSaving(self):
        if self._rewind_tv is not None:
            self._rewind_tv.restore()
            self._rewind_tv = None

def main_stack(document, create_if_missing = True):
    """main_stack(document, create_if_missing = True):returns the main TVStack instance for provided document"""    
    docname = document.Name
    
    if create_if_missing:
        if not docname in global_stacks:
            global_stacks[docname] = TVStack(document)
        
    return global_stacks.get(docname, None)

def _slotDeletedDocument(document):
    docname = document.Name
    stk = global_stacks.pop(docname, None)
    if stk is not None:
        stk.dissolve()

def _slotStartSaveDocument(doc):
    stk = main_stack(doc, create_if_missing= False)
    if stk is not None:
        stk.unwindForSaving()

def _slotFinishSaveDocument(doc):
    stk = main_stack(doc, create_if_missing= False)
    if stk is not None:
        stk.rewindAfterSaving()
