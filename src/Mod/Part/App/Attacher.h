/***************************************************************************
 *   Copyright (c) Victor Titov (DeepSOIC)                                 *
 *                                           (vv.titov@gmail.com) 2015     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/
/**
  *Attacher.h, Attacher.cpp contain the functionality of deriving placement
  *from a set of geometric subelements. Examples are: sketch attachment, datum
  *plane placement.
  */

#ifndef PARTATTACHER_H
#define PARTATTACHER_H

#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>
#include <Base/Vector3D.h>
#include <Base/Placement.h>
#include <Base/Exception.h>

#include "PartFeature.h"

#include <QString>

#include <gp_Vec.hxx>


namespace Part
{

class AttachEngine;

enum eMapMode {
    mmDeactivated,
    mmObjectXY,
    mmObjectXZ,
    mmObjectYZ,
    mmFlatFace,
    mmTangentPlane,
    mmNormalToPath,
    mmFrenetNB,
    mmFrenetTN,
    mmFrenetTB,
    mmCenterOfCurvature,
    mmThreePointsPlane,
    mmThreePointsNormal,
    mmFolding,
    mmDummy_NumberOfModes//a value useful to check the validity of mode value
};//see also eMapModeStrings[] definition in .cpp

enum eSuggestResult{
    srOK,
    srLinkBroken,
    srUnexpectedError,
    srNoModesFit,//none of the avaliable mapping modes accepts the set of topological type
    srIncompatibleGeometry,//there is a mode that could fit, but geometry is wrong (e.g. a line is required, but a curve was passed).
};

/**
 * @brief The eRefType enum lists the types of references. If adding one, see
 * also AttachEngine::getShapeType() and AttachEngine::downgradeType()
 */
enum eRefType {
    //topo              //ranks: (number of times the type is downgradable)
    rtVertex,           //1
    rtEdge,             //1
    rtFace,             //1
    rtShape,            //0
    //edges:
    rtLine,             //2
    rtCircle,           //2
    //faces:
    rtFlatFace,         //2
    rtCylindricalFace,  //2
    rtSphericalFace,    //2
    //shapes:
    rtSolid,            //1
    rtDummy_numberOfShapeTypes//a value useful to check the validity of value
};

/**
 * @brief The AttachableObject class is the thing to be inherited by an object
 * that should be attachable. It includes the required properties, and
 * shortcuts for accessing the attachment math class.
 *
 * Don't forget to add a test for property changes in mustExecute.
 */
class PartExport AttachableObject : public Part::Feature
{
    PROPERTY_HEADER(Part::AttachableObject);
public:
    AttachableObject();
    ~AttachableObject();

    /**
     * @brief setAttacher sets the AttachEngine object. The class takes the
     * ownership of the pointer, it will be deleted when the class is
     * destroyed, or when a new attacher is set. The default attacher is AttachEngine3D.
     * @param attacher. AttachableObject takes ownership and will delete it eventually.
     */
    virtual void setAttacher(AttachEngine* attacher);
    AttachEngine* attacher(void) const {return _attacher;}

    /// if the 2DObject lies on the Face of an other object this links to it
    App::PropertyLinkSubList    Support;
    App::PropertyEnumeration    MapMode; //see AttachEngine::eMapMode

    /**
      * @brief MapPathParameter is a parameter value for mmNormalToPath (the
      * sketch will be mapped normal to a curve at point specified by parameter
      * (from 0.0 to 1.0, from start to end) )
      */
    App::PropertyFloat MapPathParameter;

    /** calculate and update the Placement property based on the Support
      * this methode will calculate the position of the
      * 2D shape on a supporting Face. The Normal(Orientation) get
      * extracted from the Face and for the position an educated guess is made,
      * by examining the placement of the support object (not only the face).
      * If the support is changed this methode is called do determine a new
      * postion of the 2D shape on the supporting Face
      */
    virtual void positionBySupport(void);

public:
    void updateAttacherVals();

private:
    AttachEngine* _attacher;
};

/**
 * @brief The AttachEngine class is the placement calculation routine, modes,
 * hints and so on. It can be used separately, without deriving from
 * AttachableObject.
 */
class AttachEngine : public Base::BaseClass
{
    TYPESYSTEM_HEADER();
public: //methods
    AttachEngine();
    virtual void setUp(const App::PropertyLinkSubList &references,
                      eMapMode mapMode = mmDeactivated,
                      double attachParameter = 0.0,
                      double surfU = 0.0, double surfV = 0.0);
    virtual AttachEngine* copy() const = 0;
    virtual Base::Placement calculateAttachedPlacement(void) const = 0;

    /**
     * @brief listMapModes is the procedure that knows everything about
     * mapping modes. It returns the most appropriate mapping mode, as well as
     * list of all modes that will accept the set of references. In case no modes apply,
     * extra information regarding reasons is returned.
     * @param msg (output). Returns a message from the decision logic: OK if
     * the mode was chosen, a reason if not.
     *
     * @param allmodes (output). Pointer to a vector array that will recieve the
     * list of all modes that are applicable to the support. It doesn't
     * guarantee that all modes will work, it only checks that subelemnts are of
     * right type.
     */
    virtual eMapMode listMapModes(eSuggestResult &msg,
                                  std::vector<eMapMode>* allApplicableModes = 0,
                                  std::set<eRefType>* nextRefTypeHint = 0) const;

    /**
     * @brief getHint function returns a set of types that user can add to
     * references to arrive to combinations valid for some modes.
     *
     * @return a set of selection types that can be appended to the support.
     *
     * Subclassing: This function works out of the box via a call to
     * listMapModes, so there is no need to reimplement it.
     *
     */
    virtual const std::set<eRefType> getHint(bool forCurrentModeOnly) const;

    virtual ~AttachEngine(){};

public://helper functions that may be useful outside of the class
    static eRefType getShapeType(const TopoDS_Shape &sh);

    /**
     * @brief downgradeType converts a specific type to a topo type (e.g. rtLine -> rtEdge)
     * @param type
     * @return
     */
    static eRefType downgradeType(eRefType type);

    /**
     * @brief isShapeOfType tests if a shape fulfills the requirement of a mode, and returns a score of how spot on was the requirement.
     * @param shapeType (use return value of AttachEngine::getShapeType
     * @param requirement
     * @return : -1 - doesn't fulfill,
     *           0 - compatible topology, but incompatible specific (e.g. rtLine, rtCircle);
     *           1 - valid by generic type (e.g. rtCircle is rtEdge),
     *           2 - spot on for requirement (types are equal)
     */
    static int isShapeOfType(eRefType shapeType, eRefType requirement);


public: //enums
    static const char* eMapModeStrings[];


public: //members
    App::PropertyLinkSubList references;

    eMapMode mapMode;
    double attachParameter;
    double surfU, surfV;

    /**
     * @brief modeEnabled is an indicator, whether some mode is ever suggested
     * or not. Set to false to suppress suggesting some mode, like so:
     * modeEnabled[mmModeIDontLike] = false;
     */
    std::vector<bool> modeEnabled;

    typedef std::vector<eRefType> refTypeString;
    typedef std::vector<refTypeString> refTypeStringList;
    std::vector<refTypeStringList> modeRefTypes;

protected:
    refTypeString cat(eRefType rt1){refTypeString ret; ret.push_back(rt1); return ret;}
    refTypeString cat(eRefType rt1, eRefType rt2){refTypeString ret; ret.push_back(rt1); ret.push_back(rt2); return ret;}
    refTypeString cat(eRefType rt1, eRefType rt2, eRefType rt3){refTypeString ret; ret.push_back(rt1); ret.push_back(rt2); ret.push_back(rt3); return ret;}
    refTypeString cat(eRefType rt1, eRefType rt2, eRefType rt3, eRefType rt4){refTypeString ret; ret.push_back(rt1); ret.push_back(rt2); ret.push_back(rt3); ret.push_back(rt4); return ret;}
    void readLinks(std::vector<Part::Feature*> &parts, std::vector<const TopoDS_Shape*>& shapes, std::vector<TopoDS_Shape> &storage) const;

};


class AttachEngine3D : public AttachEngine
{
    TYPESYSTEM_HEADER();
public:
    AttachEngine3D();
    virtual AttachEngine3D* copy() const;
    virtual Base::Placement calculateAttachedPlacement(void) const;
private:
    double calculateFoldAngle(gp_Vec axA, gp_Vec axB, gp_Vec edA, gp_Vec edB) const;
};

typedef AttachEngine3D AttachEnginePlane ;//no separate class for planes, for now. Can be added later, if required.
/*
class AttachEnginePlane : public AttachEngine
{
    AttachEnginePlane();
    virtual AttachEnginePlane* copy() const {return new AttachEnginePlane(*this);}
    virtual Base::Placement calculateAttachedPlacement(void) const;
    virtual eMapMode listMapModes(eSuggestResult &msg, std::vector<eMapMode>* allmodes = 0, std::vector<QString>* nextRefTypeHint = 0) const;
    ~AttachEnginePlane(){};
};
*/

//class AttachEnginePoint : public AttachEngine
//{
//
//};


class ExceptionCancel : public Base::Exception
{
public:
    ExceptionCancel(){}
    ExceptionCancel(char* msg){this->setMessage(msg);}
    ~ExceptionCancel(){}
};

} // namespace Part

#endif // PARTATTACHER_H
