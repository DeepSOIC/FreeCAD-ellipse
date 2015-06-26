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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <TopoDS_Shape.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pln.hxx>
# include <gp_Ax1.hxx>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <GeomAPI_ProjectPointOnSurf.hxx>
# include <Geom_Plane.hxx>
# include <Geom2d_Curve.hxx>
# include <Geom2dAPI_InterCurveCurve.hxx>
# include <Geom2dAPI_ProjectPointOnCurve.hxx>
# include <GeomAPI.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
#endif
#include <BRepLProp_SLProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>

#include "Attacher.h"
#include <Base/Console.h>
#include <App/Plane.h>



using namespace Part;

const char* AttachEngine::eMapModeStrings[]= {
    "Deactivated",
    "ObjectXY",
    "ObjectXZ",
    "ObjectYZ",
    "FlatFace",
    "TangentPlane",
    "NormalToEdge",
    "FrenetNB",
    "FrenetTN",
    "FrenetTB",
    "CenterOfCurvature",
    "ThreePointsPlane",
    "ThreePointsNormal",
    "Folding",
    NULL};

PROPERTY_SOURCE(Part::AttachableObject, Part::Feature);

Part::AttachableObject::AttachableObject()
   :  _attacher(0)
{
    ADD_PROPERTY_TYPE(Support, (0,0), "Attachment",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

    //It is necessary to default to mmToFlatFace, in order to load old files
    ADD_PROPERTY_TYPE(MapMode, (mmFlatFace), "Attachment", App::Prop_None, "Mode of attachment to other object");
    MapMode.setEnums(AttachEngine::eMapModeStrings);

    ADD_PROPERTY_TYPE(MapPathParameter, (0.0), "Attachment", App::Prop_None, "Sets point of curve to map the sketch to. 0..1 = start..end");

    setAttacher(new AttachEngine3D);//default attacher
}

AttachableObject::~AttachableObject()
{
    if(_attacher)
        delete _attacher;
}

void AttachableObject::setAttacher(AttachEngine* attacher)
{
    if (_attacher)
        delete _attacher;
    _attacher = attacher;
    updateAttacherVals();
}

void AttachableObject::positionBySupport()
{
    if (!_attacher)
        return;
    updateAttacherVals();
    try{
        this->Placement.setValue(_attacher->calculateAttachedPlacement());
    } catch (ExceptionCancel) {
        //disabled, don't do anything
    };
}

void AttachableObject::updateAttacherVals()
{
    if (!_attacher)
        return;
    _attacher->setUp(this->Support, eMapMode(this->MapMode.getValue()), this->MapPathParameter.getValue(),0.0,0.0);
}

//=================================================================================

TYPESYSTEM_SOURCE_ABSTRACT(Part::AttachEngine, Base::BaseClass);

AttachEngine::AttachEngine()
{
    //by default, enable hinting of all modes.
    this->modeEnabled.resize(mmDummy_NumberOfModes,true);
    modeEnabled[mmDeactivated] = false;

    //fill type lists for modes
    modeRefTypes.resize(mmDummy_NumberOfModes);
    refTypeString s;

    s = cat(rtShape);
    modeRefTypes[mmObjectXY].push_back(s);
    modeRefTypes[mmObjectXZ].push_back(s);
    modeRefTypes[mmObjectYZ].push_back(s);

    modeRefTypes[mmFlatFace].push_back(cat(rtFlatFace));

    modeRefTypes[mmTangentPlane].push_back(cat(rtFace, rtVertex));

    s=cat(rtEdge);
    modeRefTypes[mmNormalToPath].push_back(s);
    modeRefTypes[mmFrenetNB].push_back(s);
    modeRefTypes[mmFrenetTN].push_back(s);
    modeRefTypes[mmFrenetTB].push_back(s);
    modeRefTypes[mmCenterOfCurvature].push_back(s);


    s=cat(rtEdge, rtVertex);
    modeRefTypes[mmNormalToPath].push_back(s);
    modeRefTypes[mmFrenetNB].push_back(s);
    modeRefTypes[mmFrenetTN].push_back(s);
    modeRefTypes[mmFrenetTB].push_back(s);
    modeRefTypes[mmCenterOfCurvature].push_back(s);

    modeRefTypes[mmThreePointsPlane].push_back(cat(rtVertex, rtVertex, rtVertex));
    modeRefTypes[mmThreePointsNormal].push_back(cat(rtVertex, rtVertex, rtVertex));

    modeRefTypes[mmFolding].push_back(cat(rtLine, rtLine, rtLine, rtLine));
}

void AttachEngine::setUp(const App::PropertyLinkSubList &references,
                         eMapMode mapMode,
                         double attachParameter,
                         double surfU, double surfV)
{
    this->references.Paste(references);
    this->mapMode = mapMode;
    this->attachParameter = attachParameter;
    this->surfU = surfU;
    this->surfV = surfV;
}

eMapMode AttachEngine::listMapModes(eSuggestResult& msg,
                                    std::vector<eMapMode>* allApplicableModes,
                                    std::set<eRefType>* nextRefTypeHint) const
{
    //replace a pointer with a valid reference, to avoid checks for zero pointer everywhere
    std::vector<eMapMode> buf;
    if (allApplicableModes == 0)
        allApplicableModes = &buf;
    std::vector<eMapMode> &mlist = *allApplicableModes;
    mlist.clear();
    mlist.reserve(mmDummy_NumberOfModes);

    std::set<eRefType> buf2;
    if (nextRefTypeHint == 0)
        nextRefTypeHint = &buf2;
    std::set<eRefType> &hints = *nextRefTypeHint;
    hints.clear();


    std::vector<App::GeoFeature*> parts;
    std::vector<const TopoDS_Shape*> shapes;
    std::vector<TopoDS_Shape> shapeStorage;
    try{
        readLinks(parts, shapes, shapeStorage);
    } catch (Base::Exception) {
        msg = srLinkBroken;
        return mmDeactivated;
    }

    //assemble a string representing a set of shapes
    std::vector<eRefType> typeStr;
    typeStr.resize(shapes.size());
    for( int i = 0  ;  i < shapes.size()  ;  i++){
        typeStr[i] = AttachEngine::getShapeType(*(shapes[i]));
    }

    //search valid modes.
    eMapMode bestMatchType = mmDeactivated;
    int bestMatchScore = -1;
    msg = srNoModesFit;
    for(int iMode = 0  ;  iMode < this->modeRefTypes.size()  ;  ++iMode){
        if (! this->modeEnabled[iMode])
            continue;
        const refTypeStringList &listStrings = modeRefTypes[iMode];
        for( int iStr = 0  ;  iStr < listStrings.size()  ;  ++iStr ){
            int score = 1; //-1 = topo incompatible, 0 = topo compatible, geom incompatible; 1+ = compatible (the higher - the more specific is the mode for the support)
            const refTypeString &str = listStrings[iStr];
            for (int iChr = 0  ;  iChr < str.size() && iChr < typeStr.size()  ;  ++iChr ){
                switch(AttachEngine::isShapeOfType(typeStr[iChr], str[iChr])){
                case -1:
                    score = -1;
                break;
                case 0:
                    score = 0;
                break;
                case 1:
                    //keep score
                break;
                case 2:
                    if (score > 0)
                        score++;
                break;
                }
            }

            if (score > 0  &&  str.size() > typeStr.size())
                hints.insert(str[typeStr.size()]);

            //size check is last, because we needed to collect hints
            if (str.size() != typeStr.size())
                score = -1;

            if (score > -1){//still output a best match, even if it is not completely compatible
                if (score > bestMatchScore){
                    bestMatchScore = score;
                    bestMatchType = eMapMode(iMode);
                    msg = score > 0 ? srOK : srIncompatibleGeometry;
                }
            }
            if (score > 0){
                if(mlist.size() == 0)
                    mlist.push_back(eMapMode(iMode));
                else if (mlist.back() == eMapMode(iMode))
                    mlist.push_back(eMapMode(iMode));
            }
        }
    }

    return bestMatchType;

}

const std::set<eRefType> AttachEngine::getHint(bool forCurrentModeOnly) const
{
    eSuggestResult msg;
    std::set<eRefType> ret;
    this->listMapModes(msg, 0, &ret);
    return ret;
}

eRefType AttachEngine::getShapeType(const TopoDS_Shape& sh)
{
    switch (sh.ShapeType()){
    case TopAbs_SHAPE:
        return rtShape;
    break;
    case TopAbs_SOLID:
        return rtSolid;
    break;
    case TopAbs_COMPSOLID:
    case TopAbs_COMPOUND:
    case TopAbs_SHELL:
        return rtShape;
    break;
    case TopAbs_FACE:{
        const TopoDS_Face &f = TopoDS::Face(sh);
        BRepAdaptor_Surface surf(f, /*restriction=*/Standard_False);
        switch(surf.GetType()) {
        case GeomAbs_Plane:
            return rtFlatFace;
        break;
        case GeomAbs_Cylinder:
            return rtCylindricalFace;
        break;
        case GeomAbs_Cone:
        break;
        case GeomAbs_Sphere:
            return rtSphericalFace;
        break;
        case GeomAbs_Torus:
        break;
        case GeomAbs_BezierSurface:
        break;
        case GeomAbs_BSplineSurface:
        break;
        case GeomAbs_SurfaceOfRevolution:
        break;
        case GeomAbs_SurfaceOfExtrusion:
        break;
        case GeomAbs_OffsetSurface:
        break;
        case GeomAbs_OtherSurface:
        break;
        }
        return rtFace;
    }break;
    case TopAbs_EDGE:{
        const TopoDS_Edge &e = TopoDS::Edge(sh);
        BRepAdaptor_Curve crv(e);
        switch (crv.GetType()){
        case GeomAbs_Line:
            return rtLine;
        break;
        case GeomAbs_Circle:
            return rtCircle;
        break;
        case GeomAbs_Ellipse:
        break;
        case GeomAbs_Hyperbola:
        break;
        case GeomAbs_Parabola:
        break;
        case GeomAbs_BezierCurve:
        break;
        case GeomAbs_BSplineCurve:
        break;
        case GeomAbs_OtherCurve:
        break;
        }
    }break;
    default:
        throw Base::Exception("AttachEngine::getShapeType: unexpected TopoDS_Shape::ShapeType");
    }//switch shapetype
    return rtShape;//shouldn't happen, it's here to shut up compiler warning
}

eRefType AttachEngine::downgradeType(eRefType type)
{
    switch(type){
    case rtVertex:
    case rtEdge:
    case rtFace:
        return rtShape;
    break;
    case rtShape:
        return rtShape;
    break;
    case rtLine:
    case rtCircle:
        return rtEdge;
    break;
    case rtFlatFace:
    case rtCylindricalFace:
    case rtSphericalFace:
        return rtFace;
    break;
    case rtSolid:
        return rtShape;
    break;
    default:
        throw Base::Exception("AttachEngine::downgradeType: unknown type");
    }
}

int AttachEngine::isShapeOfType(eRefType shapeType, eRefType requirement)
{
    int reqRank = 0;
    if (requirement == rtShape)
        return 1;
    else if (downgradeType(requirement) == rtShape)
        reqRank = 1;
    else if (downgradeType(downgradeType(requirement)) == rtShape)
        reqRank = 2;
    else
        assert(0);

    if (shapeType == requirement
            || downgradeType(shapeType) == requirement
            || downgradeType(downgradeType(shapeType)) == requirement)
        return reqRank == 2 ? 2 : 1;//fits

    if (reqRank == 2){
        //also test topology
        requirement = downgradeType(requirement);
        if (shapeType == requirement || downgradeType(shapeType) == requirement || downgradeType(downgradeType(shapeType)) == requirement)
            return 0;//doesn't fit, but same topology
    }
    return -1;
}

/*!
 * \brief AttachEngine3D::readLinks
 * \param parts
 * \param shapes
 * \param storage is a buffer storing what some of the pointers in shapes point to. It is needed, since subshapes are copied in the process (but copying a whole shape of an object can potentially be slow).
 */
void AttachEngine::readLinks(std::vector<App::GeoFeature*> &geofs,
                             std::vector<const TopoDS_Shape*> &shapes,
                             std::vector<TopoDS_Shape> &storage) const
{
    const std::vector<App::DocumentObject*> &objs = references.getValues();
    const std::vector<std::string> &sub = references.getSubValues();
    geofs.resize(objs.size());
    storage.reserve(objs.size());
    shapes.resize(objs.size());
    for( int i = 0  ;  i < objs.size()  ;  i++){
        if (!objs[i]->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())){
            throw Base::Exception("AttachEngine3D: link points to something that is not App::GeoFeature");
        }
        App::GeoFeature* geof = static_cast<App::GeoFeature*>(objs[i]);
        geofs[i] = geof;
        const Part::TopoShape* shape;
        if (geof->isDerivedFrom(Part::Feature::getClassTypeId())){
            shape = &(static_cast<Part::Feature*>(geof)->Shape.getShape());
            if (shape->isNull()){
                throw Base::Exception("AttachEngine3D: Part has null shape");
            }
            if (sub[i].length()>0){
                try{
                    storage.push_back(shape->getSubShape(sub[i].c_str()));
                } catch (Standard_Failure){
                    throw Base::Exception("AttachEngine3D: subshape not found");
                }
                if(storage[storage.size()-1].IsNull())
                    throw Base::Exception("AttachEngine3D: null subshape");
                shapes[i] = &(storage[storage.size()-1]);
            } else {
                shapes[i] = &(shape->_Shape);
            }
        } else if (geof->isDerivedFrom(App::Plane::getClassTypeId())) {
            Base::Vector3d norm;
            geof->Placement.getValue().getRotation().multVec(Base::Vector3d(0.0,0.0,1.0),norm);
            if (sub[0] == "back")
                norm = norm*(-1.0);
            Base::Vector3d org;
            geof->Placement.getValue().multVec(Base::Vector3d(),org);
            gp_Pln pl = gp_Pln(gp_Pnt(org.x, org.y, org.z), gp_Dir(norm.x, norm.y, norm.z));
            BRepBuilderAPI_MakeFace builder(pl);
            storage.push_back( builder.Shape() );
            shapes[i] = &(storage[storage.size()-1]);
        }


    }
}


//=================================================================================

TYPESYSTEM_SOURCE(Part::AttachEngine3D, Part::AttachEngine);

AttachEngine3D::AttachEngine3D()
{
}

AttachEngine3D* AttachEngine3D::copy() const
{
    AttachEngine3D* p = new AttachEngine3D;
    p->setUp(this->references,
             this->mapMode,
             this->attachParameter,
             this->surfU, this->surfV);
    return p;
}

Base::Placement AttachEngine3D::calculateAttachedPlacement() const
{
    const eMapMode mmode = this->mapMode;
    if (mmode == mmDeactivated)
        throw ExceptionCancel();//to be handled in positionBySupport, to not do anything if disabled
    std::vector<App::GeoFeature*> parts;
    std::vector<const TopoDS_Shape*> shapes;
    std::vector<TopoDS_Shape> copiedShapeStorage;
    readLinks(parts, shapes, copiedShapeStorage);

    if (parts.size() == 0)
        throw ExceptionCancel();


    //common stuff for all map modes
    Base::Placement Place = parts[0]->Placement.getValue();

    //variables to derive the actual placement.
    //They are to be set, depending on the mode:
     //to the sketch
    gp_Dir SketchNormal;//points at the user
    gp_Vec SketchXAxis; //if left zero, a guess will be made
    gp_Pnt SketchBasePoint; //where to put the origin of the sketch


    switch (mmode) {
    case mmDeactivated:
        //should have been filtered out already!
    break;
    case mmObjectXY:
    case mmObjectXZ:
    case mmObjectYZ:{
        //DeepSOIC: could have been done much more efficiently, but I'm lazy...
        Base::Vector3d dX,dY,dZ;//internal axes of support object, as they are in global space
        Place.getRotation().multVec(Base::Vector3d(1,0,0),dX);
        Place.getRotation().multVec(Base::Vector3d(0,1,0),dY);
        Place.getRotation().multVec(Base::Vector3d(0,0,1),dZ);
        gp_Dir dirX(dX.x, dX.y, dX.z);
        gp_Dir dirY(dY.x, dY.y, dY.z);
        gp_Dir dirZ(dZ.x, dZ.y, dZ.z);

        switch (mmode){
        case mmObjectXY:
            SketchNormal = dirZ;
            SketchXAxis = gp_Vec(dirX);
        break;
        case mmObjectXZ:
            SketchNormal = dirY.Reversed();
            SketchXAxis = gp_Vec(dirX);
        break;
        case mmObjectYZ:
            SketchNormal = dirX;
            SketchXAxis = gp_Vec(dirY);
        break;
        }
        SketchBasePoint = gp_Pnt(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);

    } break;
    case mmFlatFace:{
        if (shapes.size() < 1)
            throw Base::Exception("Part2DObject::positionBySupport: no subobjects specified (needed one planar face).");

        const TopoDS_Face &face = TopoDS::Face(*(shapes[0]));
        if (face.IsNull())
            throw Base::Exception("Null face in Part2DObject::positionBySupport()!");

        BRepAdaptor_Surface adapt(face);
        if (adapt.GetType() != GeomAbs_Plane)
            throw Base::Exception("No planar face in Part2DObject::positionBySupport()!");

        bool Reverse = false;
        if (face.Orientation() == TopAbs_REVERSED)
            Reverse = true;

        gp_Pln plane = adapt.Plane();
        Standard_Boolean ok = plane.Direct();
        if (!ok) {
            // toggle if plane has a left-handed coordinate system
            plane.UReverse();
            Reverse = !Reverse;
        }
        gp_Ax1 Normal = plane.Axis();
        if (Reverse)
            Normal.Reverse();
        SketchNormal = Normal.Direction();

        gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);

        Handle (Geom_Plane) gPlane = new Geom_Plane(plane);
        GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
        SketchBasePoint = projector.NearestPoint();

    } break;
    case mmTangentPlane: {
        if (shapes.size() < 2)
            throw Base::Exception("Part2DObject::positionBySupport: not enough subshapes (need one false and one vertex).");

        const TopoDS_Face &face = TopoDS::Face(*(shapes[0]));
        if (face.IsNull())
            throw Base::Exception("Null face in Part2DObject::positionBySupport()!");

        const TopoDS_Vertex &vertex = TopoDS::Vertex(*(shapes[1]));
        if (vertex.IsNull())
            throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");

        BRepAdaptor_Surface surf (face);
        Handle (Geom_Surface) hSurf = BRep_Tool::Surface(face);
        gp_Pnt p = BRep_Tool::Pnt(vertex);

        GeomAPI_ProjectPointOnSurf projector(p, hSurf);
        double u, v;
        if (projector.NbPoints()==0)
            throw Base::Exception("Part2DObject::positionBySupport: projecting point onto surface failed.");
        projector.LowerDistanceParameters(u, v);

        BRepLProp_SLProps prop(surf,u,v,1, Precision::Confusion());
        SketchNormal = prop.Normal();

        gp_Dir dirX;
        prop.TangentU(dirX); //if normal is defined, this should be defined too
        SketchXAxis = gp_Vec(dirX);

        if (face.Orientation() == TopAbs_REVERSED) {
            SketchNormal.Reverse();
            SketchXAxis.Reverse();
        }
        SketchBasePoint = projector.NearestPoint();
    } break;
    case mmNormalToPath:
    case mmFrenetNB:
    case mmFrenetTN:
    case mmFrenetTB:
    case mmCenterOfCurvature: {//all alignments to poing on curve
        if (shapes.size() < 1)
            throw Base::Exception("Part2DObject::positionBySupport: no subshapes specified (need one edge, and an optional vertex).");

        const TopoDS_Edge &path = TopoDS::Edge(*(shapes[0]));
        if (path.IsNull())
            throw Base::Exception("Null path in Part2DObject::positionBySupport()!");

        BRepAdaptor_Curve adapt(path);

        double u = 0.0;
        double u1 = adapt.FirstParameter();
        double u2 = adapt.LastParameter();

        //if a point is specified, use the point as a point of mapping, otherwise use parameter value from properties
        if (shapes.size() >= 2) {
            TopoDS_Vertex vertex = TopoDS::Vertex(*(shapes[1]));
            if (vertex.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
            gp_Pnt p = BRep_Tool::Pnt(vertex);

            Handle (Geom_Curve) hCurve = BRep_Tool::Curve(path, u1, u2);

            GeomAPI_ProjectPointOnCurve projector = GeomAPI_ProjectPointOnCurve (p, hCurve);
            u = projector.LowerDistanceParameter();
        } else {
            u = u1  +  this->attachParameter * (u2 - u1);
        }
        gp_Pnt p;  gp_Vec d; //point and derivative
        adapt.D1(u,p,d);

        if (d.Magnitude()<Precision::Confusion())
            throw Base::Exception("Part2DObject::positionBySupport: path curve derivative is below 1e-7, too low, can't align");

        if (mmode == mmCenterOfCurvature
                || mmode == mmFrenetNB
                || mmode == mmFrenetTN
                || mmode == mmFrenetTB){
            gp_Vec dd;//second derivative
            try{
                adapt.D2(u,p,d,dd);
            } catch (Standard_Failure &e){
                //ignore. This is brobably due to insufficient continuity.
                dd = gp_Vec(0., 0., 0.);
                Base::Console().Warning("Part2DObject::positionBySupport: can't calculate second derivative of curve. OCC error: %s\n", e.GetMessageString());
            }

            gp_Vec T,N,B;//Frenet?Serret axes: tangent, normal, binormal
            T = d.Normalized();
            N = dd.Subtracted(T.Multiplied(dd.Dot(T)));//take away the portion of dd that is along tangent
            if (N.Magnitude() > Precision::SquareConfusion()) {
                N.Normalize();
                B = T.Crossed(N);
            } else {
                Base::Console().Warning("Part2DObject::positionBySupport: path curve second derivative is below 1e-14, can't align x axis.\n");
                N = gp_Vec(0.,0.,0.);
                B = gp_Vec(0.,0.,0.);//redundant, just for consistency
            }

            SketchBasePoint = p; //align sketch origin to the point the curve pierces the sketch. In mmNormalToPathRev, it will be overwritten later

            switch (mmode){
            case mmFrenetNB:
            case mmCenterOfCurvature:
                SketchNormal = T.Reversed();//to avoid sketches upside-down for regular curves like circles
                SketchXAxis = N.Reversed();
                if (mmode == mmCenterOfCurvature) {
                    //make sketch Y axis of sketch be the axis of osculating circle
                    if (N.Magnitude() == 0.0)
                        throw Base::Exception("Part2DObject::positionBySupport: path has infinite radius of curvature at the point. Can't align for revolving.");
                    double curvature = dd.Dot(N) / pow(d.Magnitude(), 2);
                    gp_Vec pv (p.XYZ());
                    pv.Add(N.Multiplied(1/curvature));//shift the point along curvature by radius of curvature
                    SketchBasePoint = gp_Pnt(pv.XYZ());
                    //it would have been cool to have the curve attachment point available inside sketch... Leave for future.
                }
            break;
            case mmFrenetTN:
                if (N.Magnitude() == 0.0)
                    throw Base::Exception("Part2DObject::positionBySupport: Frenet-Serret normal is undefined. Can't align to TN plane.");
                SketchNormal = B;
                SketchXAxis = T;
            break;
            case mmFrenetTB:
                if (N.Magnitude() == 0.0)
                    throw Base::Exception("Part2DObject::positionBySupport: Frenet-Serret normal is undefined. Can't align to TB plane.");
                SketchNormal = N.Reversed();//it is more convenient to sketch on something looking it it so it is convex.
                SketchXAxis = T;
            break;
            default:
                assert(0);
            }
        } else if (mmode == mmNormalToPath){//mmNormalToPath
            //align sketch origin to the origin of support
            SketchNormal = gp_Dir(d.Reversed());//sketch normal looks at user. It is natural to have the curve directed away from user, so reversed.
            gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);
            Handle (Geom_Plane) gPlane = new Geom_Plane(p, SketchNormal);
            GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
            SketchBasePoint = projector.NearestPoint();
        }

    } break;
    case mmThreePointsPlane:
    case mmThreePointsNormal: {
        if (shapes.size() < 3)
            throw Base::Exception("Part2DObject::positionBySupport: less than 3 subshapes specified (need three vertices).");

        const TopoDS_Vertex &vertex0 = TopoDS::Vertex(*(shapes[0]));
        if (vertex0.IsNull())
            throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
        const TopoDS_Vertex &vertex1 = TopoDS::Vertex(*(shapes[1]));
        if (vertex1.IsNull())
            throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
        const TopoDS_Vertex &vertex2 = TopoDS::Vertex(*(shapes[2]));
        if (vertex2.IsNull())
            throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");

        gp_Pnt p0 = BRep_Tool::Pnt(vertex0);
        gp_Pnt p1 = BRep_Tool::Pnt(vertex1);
        gp_Pnt p2 = BRep_Tool::Pnt(vertex2);

        gp_Vec vec01 (p0,p1);
        gp_Vec vec02 (p0,p2);
        if (vec01.Magnitude() < Precision::Confusion() || vec02.Magnitude() < Precision::Confusion())
            throw Base::Exception("Part2DObject::positionBySupport: some of 3 points are coincident. Can't make a plane");
        vec01.Normalize();
        vec02.Normalize();

        gp_Vec norm ;
        if (mmode == mmThreePointsPlane) {
            norm = vec01.Crossed(vec02);
            if (norm.Magnitude() < Precision::Confusion())
                throw Base::Exception("Part2DObject::positionBySupport: points are collinear. Can't make a plane");
        } else if (mmode == mmThreePointsNormal) {
            norm = vec02.Subtracted(vec01.Multiplied(vec02.Dot(vec01))).Reversed();//norm = vec02 forced perpendicular to vec01.
            if (norm.Magnitude() < Precision::Confusion())
                throw Base::Exception("Part2DObject::positionBySupport: points are collinear. Can't make a plane");
        }

        norm.Normalize();
        SketchNormal = gp_Dir(norm);

        gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);

        Handle (Geom_Plane) gPlane = new Geom_Plane(p0,SketchNormal);
        GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
        SketchBasePoint = projector.NearestPoint();
    } break;
    case mmFolding: {

        // Expected selection: four edges in order: edgeA, fold axis A,
        // fold axis B, edgeB. The sketch will be placed angled so as to join
        // edgeA to edgeB by folding the sheet along axes. All edges are
        // expected to be in one plane.

        if (shapes.size()<4)
            throw Base::Exception("Part2DObject::positionBySupport: not enough shapes (need 4 lines: edgeA, axisA, axisB, edgeB).");

        //extract the four lines
        const TopoDS_Edge* (edges[4]);
        BRepAdaptor_Curve adapts[4];
        gp_Lin lines[4];
        for(int i=0  ;  i<4  ;  i++){
            edges[i] = &TopoDS::Edge(*(shapes[i]));
            if (edges[i]->IsNull())
                throw Base::Exception("Null edge in Part2DObject::positionBySupport()!");

            adapts[i] = BRepAdaptor_Curve(*(edges[i]));
            if (adapts[i].GetType() != GeomAbs_Line)
                throw Base::Exception("Part2DObject::positionBySupport: Folding - non-straight edge.");
            lines[i] = adapts[i].Line();
        }

        //figure out the common starting point
        gp_Pnt p, p1, p2, p3, p4;
        double signs[4] = {0,0,0,0};//flags whether to reverse line directions, for all directions to point away from the common vertex
        p1 = adapts[0].Value(adapts[0].FirstParameter());
        p2 = adapts[0].Value(adapts[0].LastParameter());
        p3 = adapts[1].Value(adapts[1].FirstParameter());
        p4 = adapts[1].Value(adapts[1].LastParameter());
        p = p1;
        if (p1.Distance(p3) < Precision::Confusion()){
            p = p3;
            signs[0] = +1.0;
            signs[1] = +1.0;
        } else if (p1.Distance(p4) < Precision::Confusion()){
            p = p4;
            signs[0] = +1.0;
            signs[1] = -1.0;
        } else if (p2.Distance(p3) < Precision::Confusion()){
            p = p3;
            signs[0] = -1.0;
            signs[1] = +1.0;
        } else if (p2.Distance(p4) < Precision::Confusion()){
            p = p4;
            signs[0] = -1.0;
            signs[1] = -1.0;
        } else {
            throw Base::Exception("Part2DObject::positionBySupport: Folding - edges to not share a vertex.");
        }
        for (int i = 2  ;  i<4  ;  i++){
            p1 = adapts[i].Value(adapts[i].FirstParameter());
            p2 = adapts[i].Value(adapts[i].LastParameter());
            if (p.Distance(p1) < Precision::Confusion())
                signs[i] = +1.0;
            else if (p.Distance(p2) < Precision::Confusion())
                signs[i] = -1.0;
            else
                throw Base::Exception("Part2DObject::positionBySupport: Folding - edges to not share a vertex.");
        }

        gp_Vec dirs[4];
        for(int i=0  ;  i<4  ;  i++){
            assert(abs(signs[i]) == 1.0);
            dirs[i] = gp_Vec(lines[i].Direction()).Multiplied(signs[i]);
        }

        double ang = this->calculateFoldAngle(
                    dirs[1],
                    dirs[2],
                    dirs[0],
                    dirs[3]
                );

        gp_Vec norm = dirs[1].Crossed(dirs[2]);
        norm.Rotate(gp_Ax1(gp_Pnt(),gp_Dir(dirs[1])),-ang);//rotation direction: when angle is positive, rotation is CCW when observing the vector so that the axis is pointing at you. Hence angle is negated here.
        SketchNormal = norm.Reversed();

        SketchXAxis = dirs[1];

        gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);
        Handle (Geom_Plane) gPlane = new Geom_Plane(p, SketchNormal);
        GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
        SketchBasePoint = projector.NearestPoint();

    } break;
    default:
        assert(0/*Attachment mode is not implemented?*/);
        Base::Console().Error("Attachment mode %i is not implemented.\n", int(mmode));
        throw ExceptionCancel();
    }//switch (MapMode)

    //----------calculate placement, based on point and vector.

    gp_Ax3 SketchPos;
    if (SketchXAxis.Magnitude() > Precision::Confusion()) {
        SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, SketchXAxis);
    } else {
        //find out, to which axis of support Normal is closest to.
        //The result will be written into pos variable (0..2 = X..Z)
        Base::Vector3d dX,dY,dZ;//internal axes of support object, as they are in global space
        Place.getRotation().multVec(Base::Vector3d(1,0,0),dX);
        Place.getRotation().multVec(Base::Vector3d(0,1,0),dY);
        Place.getRotation().multVec(Base::Vector3d(0,0,1),dZ);
        gp_Dir dirX(dX.x, dX.y, dX.z);
        gp_Dir dirY(dY.x, dY.y, dY.z);
        gp_Dir dirZ(dZ.x, dZ.y, dZ.z);
        double cosNX = SketchNormal.Dot(dirX);
        double cosNY = SketchNormal.Dot(dirY);
        double cosNZ = SketchNormal.Dot(dirZ);
        std::vector<double> cosXYZ;
        cosXYZ.push_back(fabs(cosNX));
        cosXYZ.push_back(fabs(cosNY));
        cosXYZ.push_back(fabs(cosNZ));

        int pos = std::max_element(cosXYZ.begin(), cosXYZ.end()) - cosXYZ.begin();

        // +X/-X
        if (pos == 0) {
            if (cosNX > 0)
                SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, dirY);
            else
                SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, -dirY);
        }
        // +Y/-Y
        else if (pos == 1) {
            if (cosNY > 0)
                SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, -dirX);
            else
                SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, dirX);
        }
        // +Z/-Z
        else {
            SketchPos = gp_Ax3(SketchBasePoint, SketchNormal, dirX);
        }
    } // if SketchXAxis.Magnitude() > Precision::Confusion

    gp_Trsf Trf;
    Trf.SetTransformation(SketchPos);
    Trf.Invert();
    Trf.SetScaleFactor(Standard_Real(1.0));

    Base::Matrix4D mtrx;
    TopoShape::convertToMatrix(Trf,mtrx);

    return Base::Placement(mtrx);
}

double AttachEngine3D::calculateFoldAngle(gp_Vec axA, gp_Vec axB, gp_Vec edA, gp_Vec edB) const
{
    //DeepSOIC: this hardcore math can probably be replaced with a couple of
    //clever OCC calls... See forum thread "Sketch mapping enhancement" for a
    //picture on how this math was derived.
    //http://forum.freecadweb.org/viewtopic.php?f=8&t=10511&sid=007946a934530ff2a6c9259fb32624ec&start=40#p87584
    axA.Normalize();
    axB.Normalize();
    edA.Normalize();
    edB.Normalize();
    gp_Vec norm = axA.Crossed(axB);
    if (norm.Magnitude() < Precision::Confusion())
        throw Base::Exception("calculateFoldAngle: Folding axes are parallel, folding angle cannot be computed.");
    norm.Normalize();
    double a = edA.Dot(axA);
    double ra = edA.Crossed(axA).Magnitude();
    if (abs(ra) < Precision::Confusion())
        throw Base::Exception("calculateFoldAngle: axisA and edgeA are parallel, folding can't be computed.");
    double b = edB.Dot(axB);
    double rb = edB.Crossed(axB).Magnitude();
    double costheta = axB.Dot(axA);
    double sintheta = axA.Crossed(axB).Dot(norm);
    double singama = -costheta;
    double cosgama = sintheta;
    double k = b*cosgama;
    double l = a + b*singama;
    double xa = k + l*singama/cosgama;
    double cos_unfold = -xa/ra;
    if (abs(cos_unfold)>0.999)
        throw Base::Exception("calculateFoldAngle: cosine of folding angle is too close to or above 1.");
    return acos(cos_unfold);
}



