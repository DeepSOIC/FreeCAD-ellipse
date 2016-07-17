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

__title__="BOPTools.SplitFeatures module"
__author__ = "DeepSOIC"
__url__ = "http://www.freecadweb.org"
__doc__ = "Shape splitting document objects (features)."

from . import ShapeMerge
import FreeCAD
import Part

if FreeCAD.GuiUp:
    import FreeCADGui
    from PySide import QtCore, QtGui

#-------------------------- translation-related code ----------------------------------------
#(see forum thread "A new Part tool is being born... JoinFeatures!"
#http://forum.freecadweb.org/viewtopic.php?f=22&t=11112&start=30#p90239 )
try:
    _fromUtf8 = QtCore.QString.fromUtf8
except Exception:
    def _fromUtf8(s):
        return s
try:
    _encoding = QtGui.QApplication.UnicodeUTF8
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig, _encoding)
except NameError:
    def _translate(context, text, disambig):
        return QtGui.QApplication.translate(context, text, disambig)
#--------------------------/translation-related code ----------------------------------------

def cmdCreateBooleanFragmentsFeature(name, mode):
    """cmdCreateBooleanFragmentsFeature(name, mode): implementation of GUI command to create 
    BooleanFragments feature (GFA). Mode can be "Standard", or "CompSolid"."""
    sel = FreeCADGui.Selection.getSelectionEx()
    FreeCAD.ActiveDocument.openTransaction("Create Inerference")
    FreeCADGui.addModule("BOPTools.SplitFeatures")
    FreeCADGui.doCommand("j = BOPTools.SplitFeatures.makeBooleanFragments(name= '{name}')".format(name= name))
    FreeCADGui.doCommand("j.Objects = {sel}".format(
       sel= "["  +  ", ".join(["App.ActiveDocument."+so.Object.Name for so in sel])  +  "]"
       ))
    FreeCADGui.doCommand("j.Mode = {mode}".format(mode= repr(mode)))

    try:
        FreeCADGui.doCommand("j.Proxy.execute(j)")
        FreeCADGui.doCommand("j.purgeTouched()")
    except Exception as err:
        mb = QtGui.QMessageBox()
        mb.setIcon(mb.Icon.Warning)
        mb.setText(_translate("Part_SplitFeatures","Computing the result failed with an error: {err}. Click 'Continue' to create the feature anyway, or 'Abort' to cancel.", None)
                   .format(err= err.message))
        mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
        btnAbort = mb.addButton(QtGui.QMessageBox.StandardButton.Abort)
        btnOK = mb.addButton(_translate("Part_SplitFeatures","Continue",None), QtGui.QMessageBox.ButtonRole.ActionRole)
        mb.setDefaultButton(btnOK)

        mb.exec_()

        if mb.clickedButton() is btnAbort:
            FreeCAD.ActiveDocument.abortTransaction()
            return

    FreeCADGui.doCommand("for obj in j.ViewObject.Proxy.claimChildren():\n"
                         "    obj.ViewObject.hide()")

    FreeCAD.ActiveDocument.commitTransaction()

def getIconPath(icon_dot_svg):
    return ":/icons/" + icon_dot_svg

# -------------------------- /common stuff --------------------------------------------------

# -------------------------- BooleanFragments --------------------------------------------------

def makeBooleanFragments(name):
    '''makeBooleanFragments(name): makes an BooleanFragments object.'''
    obj = FreeCAD.ActiveDocument.addObject("Part::FeaturePython",name)
    FeatureBooleanFragments(obj)
    if FreeCAD.GuiUp:
        ViewProviderBooleanFragments(obj.ViewObject)
    return obj

class FeatureBooleanFragments:
    "The BooleanFragments feature object"
    def __init__(self,obj):
        obj.addProperty("App::PropertyLinkList","Objects","BooleanFragments","Object to compute intersections between.")
        obj.addProperty("App::PropertyEnumeration","Mode","BooleanFragments","Standard: wires, shells, compsolids remain in one piece. Split: wires, shells, compsolids are split. CompSolid: make compsolid from solid fragments.")
        obj.Mode = ["Standard", "Split", "CompSolid"]

        obj.Proxy = self

    def execute(self,selfobj):
        shapes = [obj.Shape for obj in selfobj.Objects]
        if len(shapes) == 1 and shapes[0].ShapeType == "Compound":
            shapes = shapes[0].childShapes()
        if len(shapes) < 2:
            raise ValueError("At least two shapes are needed for computing boolean fragments. Got only {num}.".format(num= len(shapes)))
        pieces, map = shapes[0].generalFuse(shapes[1:])
        if selfobj.Mode == "Standard":
            selfobj.Shape = pieces
        elif selfobj.Mode == "CompSolid":
            solids = pieces.Solids
            if len(solids) < 1:
                raise ValueError("No solids in the result. Can't make CompSolid.")
            elif len(solids) == 1:
                FreeCAD.Console.PrintWarning("Part_BooleanFragments: only one solid in the result, generating trivial compsolid.")
            selfobj.Shape = ShapeMerge.mergeSolids(solids, bool_compsolid= True)
        elif selfobj.Mode == "Split":
            from .GeneralFuseResult import GeneralFuseResult
            gr = GeneralFuseResult(shapes, (pieces,map))
            gr.explodeCompounds()
            gr.splitWiresShells()
            selfobj.Shape = Part.Compound(gr.pieces)


class ViewProviderBooleanFragments:
    "A View Provider for the Part BooleanFragments feature"

    def __init__(self,vobj):
        vobj.Proxy = self

    def getIcon(self):
        return getIconPath("Part_BooleanFragments.svg")

    def attach(self, vobj):
        self.ViewObject = vobj
        self.Object = vobj.Object


    def setEdit(self,vobj,mode):
        return False

    def unsetEdit(self,vobj,mode):
        return

    def __getstate__(self):
        return None

    def __setstate__(self,state):
        return None

    def claimChildren(self):
        return self.Object.Objects

    def onDelete(self, feature, subelements):
        try:
            for obj in self.claimChildren():
                obj.ViewObject.show()
        except Exception as err:
            FreeCAD.Console.PrintError("Error in onDelete: " + err.message)
        return True

class CommandBooleanFragments:
    "Command to create BooleanFragments feature"
    def GetResources(self):
        return {'Pixmap'  : getIconPath("Part_BooleanFragments.svg"),
                'MenuText': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","BooleanFragments"),
                'Accel': "",
                'ToolTip': QtCore.QT_TRANSLATE_NOOP("Part_SplitFeatures","Part_BooleanFragments: split objects where they intersect")}

    def Activated(self):
        if len(FreeCADGui.Selection.getSelectionEx()) >= 1 :
            cmdCreateBooleanFragmentsFeature(name= "BooleanFragments", mode= "Standard")
        else:
            mb = QtGui.QMessageBox()
            mb.setIcon(mb.Icon.Warning)
            mb.setText(_translate("Part_SplitFeatures", "Select at least two objects, or one or more compounds, first! If only one compound is selected, the compounded shapes will be intersected between each other (otherwise, compounds with self-intersections are invalid).", None))
            mb.setWindowTitle(_translate("Part_SplitFeatures","Bad selection", None))
            mb.exec_()

    def IsActive(self):
        if FreeCAD.ActiveDocument:
            return True
        else:
            return False

# -------------------------- /BooleanFragments --------------------------------------------------

def addCommands():
    FreeCADGui.addCommand('Part_BooleanFragments',CommandBooleanFragments())
    