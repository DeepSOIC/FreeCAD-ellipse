/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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
#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif


#include <Base/Exception.h>
#include "Part2DObject.h"
#include "Geometry.h"

#include <App/FeaturePythonPyImp.h>
#include "Part2DObjectPy.h"

#include <BRepLProp_SLProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>


using namespace Part;

const int Part2DObject::H_Axis = -1;
const int Part2DObject::V_Axis = -2;
const int Part2DObject::N_Axis = -3;

const char* Part2DObject::eMapModeStrings[]= {
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


PROPERTY_SOURCE(Part::Part2DObject, Part::Feature)


Part2DObject::Part2DObject()
{

     ADD_PROPERTY_TYPE(Support,(0),   "Attachment",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

     //It is necessary to default to mmToFlatFace, in order to load old files
     ADD_PROPERTY_TYPE(MapMode, (mmFlatFace), "Attachment", App::Prop_None, "Mode of attachment to other object");
     MapMode.setEnums(eMapModeStrings);

     ADD_PROPERTY_TYPE(MapPathParameter, (0.0), "Attachment", App::Prop_None, "Sets point of curve to map the sketch to. 0..1 = start..end");

}


App::DocumentObjectExecReturn *Part2DObject::execute(void)
{
    return App::DocumentObject::StdReturn;
}

double Part2DObject::calculateFoldAngle(gp_Vec axA, gp_Vec axB, gp_Vec edA, gp_Vec edB)
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

void Part2DObject::positionBySupport(void)
{
    // recalculate support:
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return;


    const eMapMode mmode = eMapMode(this->MapMode.getValue());

    if (mmode != mmDeactivated) {
        //common stuff for all map modes
        Base::Placement Place = part->Placement.getValue();
        const std::vector<std::string> &sub = Support.getSubValues();
        // get the selected sub shape (a Face)
        const Part::TopoShape &shape = part->Shape.getShape();
        if (shape._Shape.IsNull())
            throw Base::Exception("Support shape is empty!");

        //get all subshapes
        std::vector<TopoDS_Shape> shapes;
        shapes.reserve(4);
        for (int i=0; i<sub.size(); i++) {
            try {
                shapes.push_back(shape.getSubShape(sub[i].c_str()));
            }
            catch (Standard_Failure) {
                throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
            }
        }

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
            if (sub.size() < 1)
                throw Base::Exception("Part2DObject::positionBySupport: no subobjects specified (needed one planar face).");

            const TopoDS_Face &face = TopoDS::Face(shapes[0]);
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
            if (sub.size() < 2)
                throw Base::Exception("Part2DObject::positionBySupport: not enough subshapes (need one false and one vertex).");

            const TopoDS_Face &face = TopoDS::Face(shapes[0]);
            if (face.IsNull())
                throw Base::Exception("Null face in Part2DObject::positionBySupport()!");

            const TopoDS_Vertex &vertex = TopoDS::Vertex(shapes[1]);
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
            if (sub.size() < 1)
                throw Base::Exception("Part2DObject::positionBySupport: no subshapes specified (need one edge, and an optional vertex).");

            const TopoDS_Edge &path = TopoDS::Edge(shapes[0]);
            if (path.IsNull())
                throw Base::Exception("Null path in Part2DObject::positionBySupport()!");

            BRepAdaptor_Curve adapt(path);

            double u = 0.0;
            double u1 = adapt.FirstParameter();
            double u2 = adapt.LastParameter();

            //if a point is specified, use the point as a point of mapping, otherwise use parameter value from properties
            if (sub.size() >= 2) {
                TopoDS_Vertex vertex = TopoDS::Vertex(shapes[1]);
                if (vertex.IsNull())
                    throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
                gp_Pnt p = BRep_Tool::Pnt(vertex);

                Handle (Geom_Curve) hCurve = BRep_Tool::Curve(path, u1, u2);

                GeomAPI_ProjectPointOnCurve projector = GeomAPI_ProjectPointOnCurve (p, hCurve);
                u = projector.LowerDistanceParameter();
            } else {
                u = u1  +  this->MapPathParameter.getValue() * (u2 - u1);
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
            if (sub.size() < 3)
                throw Base::Exception("Part2DObject::positionBySupport: less than 3 subshapes specified (need three vertices).");

            const TopoDS_Vertex &vertex0 = TopoDS::Vertex(shapes[0]);
            if (vertex0.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
            const TopoDS_Vertex &vertex1 = TopoDS::Vertex(shapes[1]);
            if (vertex1.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
            const TopoDS_Vertex &vertex2 = TopoDS::Vertex(shapes[2]);
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

            if (sub.size()<4)
                throw Base::Exception("Part2DObject::positionBySupport: not enough shapes (need 4 lines: edgeA, axisA, axisB, edgeB).");

            //extract the four lines
            TopoDS_Edge* (edges[4]);
            BRepAdaptor_Curve adapts[4];
            gp_Lin lines[4];
            for(int i=0  ;  i<4  ;  i++){
                edges[i] = &TopoDS::Edge(shapes[i]);
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
            Base::Console().Error("Attachment mode %s is not implemented.\n", this->MapMode.getValueAsString());
            //don't throw - just ignore...
            return;
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

        this->Placement.setValue(Base::Placement(mtrx));

    }//if not disabled
}

Part2DObject::eMapMode Part2DObject::SuggestAutoMapMode(const App::PropertyLinkSub& Support, eSuggestResult &msg, std::vector<eMapMode>* allApplicableModes)
{

    //replace a pointer with a valid reference, to avoid chacks for zero pointer everywhere
    std::vector<eMapMode> buf;
    if (allApplicableModes == 0)
        allApplicableModes = &buf;
    std::vector<eMapMode> &mlist = *allApplicableModes;
    mlist.clear();
    mlist.reserve(mmDummy_NumberOfModes);

    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())){
        msg = srLinkBroken;
        return mmDeactivated;
    }
    const Part::TopoShape &shape = part->Shape.getShape();
    if (shape.isNull()){
        msg = srLinkBroken;
        return mmDeactivated;
    }

    const std::vector<std::string> &sub = Support.getSubValues();
    std::string typeList;
    for (int i=0; i<sub.size(); i++) {
        char c = (sub[i])[0];
        if (c == 0)
            c = '.';
        typeList += c;
    }

    std::vector<TopoDS_Shape> shapes;
    shapes.reserve(4);

    for (int i=0; i<sub.size(); i++) {
        try {
            shapes.push_back(shape.getSubShape(sub[i].c_str()));
        }
        catch (Standard_Failure) {
            msg = srLinkBroken;
            return mmDeactivated;
        }
    }


    try{
        msg = srOK;
        //F = face, E = edge, V = vertex
        if (typeList == ""){
            mlist.push_back(mmObjectXY);
            mlist.push_back(mmObjectXZ);
            mlist.push_back(mmObjectYZ);
            return mmObjectXY;
        }else if (typeList == "F") {
            //check if the face is planar. If what follows throws, it's not.
            TopoDS_Face &f = TopoDS::Face(shapes[0]);
            BRepAdaptor_Surface adapt(f);
            msg = srNonPlanarFace;
            adapt.Plane();
            msg = srOK;
            mlist.push_back(mmFlatFace);
            return mmFlatFace;
        } else if (typeList == "FV") {
            mlist.push_back(mmTangentPlane);
            return mmTangentPlane;
        } else if (typeList == "E" || typeList == "EV") {
            TopoDS_Edge &e = TopoDS::Edge(shapes[0]);
            BRepAdaptor_Curve adapt(e);
            mlist.push_back(mmNormalToPath);
            if (adapt.GetType() != GeomAbs_Line) {
                mlist.push_back(mmFrenetNB);
                mlist.push_back(mmFrenetTN);
                mlist.push_back(mmFrenetTB);
                mlist.push_back(mmCenterOfCurvature);
            }
            if (adapt.GetType() == GeomAbs_Circle)
                return mmCenterOfCurvature;
            else
                return mmNormalToPath;
        } else if (typeList == "VVV") {
            mlist.push_back(mmThreePointsPlane);
            mlist.push_back(mmThreePointsNormal);
            return mmThreePointsPlane;
        } else if (typeList == "EEEE") {
            //check if the edges are straight. If what follows throws - they are not.
            assert(shapes.size() == 4);
            for (int i = 0  ;  i < 4  ;  i++){
                TopoDS_Edge &e = TopoDS::Edge(shapes[i]);
                BRepAdaptor_Curve adapt(e);
                msg = srNonStraightEdge;
                adapt.Line();
                msg = srOK;
            }
            mlist.push_back(mmFolding);
            return mmFolding;
        } else {
            msg = srNoModesFit;
            return mmDeactivated;
        }
    } catch (Standard_Failure) {
        if (msg == srOK)
            msg = srUnexpectedError;
        return mmDeactivated;
    }
}

void Part2DObject::transformPlacement(const Base::Placement &transform)
{
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (part && part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        part->transformPlacement(transform);
        positionBySupport();
    } else
        GeoFeature::transformPlacement(transform);
}

int Part2DObject::getAxisCount(void) const
{
    return 0;
}

Base::Axis Part2DObject::getAxis(int axId) const
{
    if (axId == H_Axis) {
        return Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(1,0,0));
    }
    else if (axId == V_Axis) {
        return Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,1,0));
    }
    else if (axId == N_Axis) {
        return Base::Axis(Base::Vector3d(0,0,0), Base::Vector3d(0,0,1));
    }
    return Base::Axis();
}

bool Part2DObject::seekTrimPoints(const std::vector<Geometry *> &geomlist,
                                  int GeoId, const Base::Vector3d &point,
                                  int &GeoId1, Base::Vector3d &intersect1,
                                  int &GeoId2, Base::Vector3d &intersect2)
{
    if (GeoId >= int(geomlist.size()))
        return false;

    gp_Pln plane(gp_Pnt(0,0,0),gp_Dir(0,0,1));

    Standard_Boolean periodic=Standard_False;
    double period;
    Handle_Geom2d_Curve primaryCurve;
    Handle_Geom_Geometry geom = (geomlist[GeoId])->handle();
    Handle_Geom_Curve curve3d = Handle_Geom_Curve::DownCast(geom);
    if (curve3d.IsNull())
        return false;
    else {
        primaryCurve = GeomAPI::To2d(curve3d, plane);
        periodic = primaryCurve->IsPeriodic();
        if (periodic)
            period = primaryCurve->Period();
    }

    // create the intersector and projector functions
    Geom2dAPI_InterCurveCurve Intersector;
    Geom2dAPI_ProjectPointOnCurve Projector;

    // find the parameter of the picked point on the primary curve
    Projector.Init(gp_Pnt2d(point.x, point.y), primaryCurve);
    double pickedParam = Projector.LowerDistanceParameter();

    // find intersection points
    GeoId1 = -1;
    GeoId2 = -1;
    double param1=-1e10,param2=1e10;
    gp_Pnt2d p1,p2;
    Handle_Geom2d_Curve secondaryCurve;
    for (int id=0; id < int(geomlist.size()); id++) {
        // #0000624: Trim tool doesn't work with construction lines
        if (id != GeoId/* && !geomlist[id]->Construction*/) {
            geom = (geomlist[id])->handle();
            curve3d = Handle_Geom_Curve::DownCast(geom);
            if (!curve3d.IsNull()) {
                secondaryCurve = GeomAPI::To2d(curve3d, plane);
                // perform the curves intersection
                Intersector.Init(primaryCurve, secondaryCurve, 1.0e-12);
                for (int i=1; i <= Intersector.NbPoints(); i++) {
                    gp_Pnt2d p = Intersector.Point(i);
                    // get the parameter of the intersection point on the primary curve
                    Projector.Init(p, primaryCurve);
                    double param = Projector.LowerDistanceParameter();
                    if (periodic) {
                        // transfer param into the interval (pickedParam-period pickedParam]
                        param = param - period * ceil((param-pickedParam) / period);
                        if (param > param1) {
                            param1 = param;
                            p1 = p;
                            GeoId1 = id;
                        }
                        param -= period; // transfer param into the interval (pickedParam pickedParam+period]
                        if (param < param2) {
                            param2 = param;
                            p2 = p;
                            GeoId2 = id;
                        }
                    }
                    else if (param < pickedParam && param > param1) {
                        param1 = param;
                        p1 = p;
                        GeoId1 = id;
                    }
                    else if (param > pickedParam && param < param2) {
                        param2 = param;
                        p2 = p;
                        GeoId2 = id;
                    }
                }
            }
        }
    }

    if (periodic) {
        // in case both points coincide, cancel the selection of one of both
        if (abs(param2-param1-period) < 1e-10) {
            if (param2 - pickedParam >= pickedParam - param1)
                GeoId2 = -1;
            else
                GeoId1 = -1;
        }
    }

   if (GeoId1 < 0 && GeoId2 < 0)
       return false;

   if (GeoId1 >= 0)
       intersect1 = Base::Vector3d(p1.X(),p1.Y(),0.f);
   if (GeoId2 >= 0)
       intersect2 = Base::Vector3d(p2.X(),p2.Y(),0.f);
   return true;
}

void Part2DObject::acceptGeometry()
{
    // implemented in sub-classes
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
  PROPERTY_SOURCE_TEMPLATE(Part::Part2DObjectPython, Part::Part2DObject)
  template<> const char* Part::Part2DObjectPython::getViewProviderName(void) const {
    return "PartGui::ViewProvider2DObjectPython";
  }
  template<> PyObject* Part::Part2DObjectPython::getPyObject(void) {
        if (PythonObject.is(Py::_None())) {
            // ref counter is set to 1
            PythonObject = Py::Object(new FeaturePythonPyT<Part::Part2DObjectPy>(this),true);
        }
        return Py::new_reference_to(PythonObject);
  }
/// @endcond

// explicit template instantiation
  template class PartExport FeaturePythonT<Part::Part2DObject>;
}

