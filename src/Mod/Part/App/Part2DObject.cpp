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
    "To Flat Face",
    "Tangent plane",
    "Normal to path",
    "FrenetNB",
    "FrenetTN",
    "FrenetTB",
    "CenterOfCurvature",
    "On 3 points",
    NULL};


PROPERTY_SOURCE(Part::Part2DObject, Part::Feature)


Part2DObject::Part2DObject()
{

     ADD_PROPERTY_TYPE(Support,(0),   "2D",(App::PropertyType)(App::Prop_None),"Support of the 2D geometry");

     //It is necessary to default to mmToFlatFace, in order to load old files
     ADD_PROPERTY_TYPE(MapMode, (mmToFlatFace), "2D", App::Prop_None, "Mode of attachment to other object");
     MapMode.setEnums(eMapModeStrings);

     ADD_PROPERTY_TYPE(MapPathParameter, (0.0), "2D", App::Prop_None, "Sets point of curve to map the sketch to. 0..1 = start..end");

}


App::DocumentObjectExecReturn *Part2DObject::execute(void)
{
    return App::DocumentObject::StdReturn;
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

        TopoDS_Shape sh0; //the first subshape listed in Support.getSubValues. Can be empty, if wasn't specified.
        try {
            if (sub.size() > 0)
                sh0 = shape.getSubShape(sub[0].c_str());
        }
        catch (Standard_Failure) {
            throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
        }

        //variables to derive the actual placement.
        //They are to be set, depending on the mode:
         //to the sketch
        gp_Dir SketchNormal;
        gp_Vec SketchXAxis; //if left zero, a guess will be made
        gp_Pnt SketchBasePoint; //where to put the origin of the sketch


        switch (mmode) {
        case mmDeactivated:
            //should have been filtered out already!
        break;
        case mmToFlatFace:{
            const TopoDS_Face &face = TopoDS::Face(sh0);
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
                throw Base::Exception("Part2DObject::positionBySupport: second subshape is not specified for TangentPlane alignment mode.");
            TopoDS_Shape sh1;
            try {
                sh1 = shape.getSubShape(sub[1].c_str());
            }
            catch (Standard_Failure) {
                throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
            }

            const TopoDS_Face &face = TopoDS::Face(sh0);
            if (face.IsNull())
                throw Base::Exception("Null face in Part2DObject::positionBySupport()!");

            const TopoDS_Vertex &vertex = TopoDS::Vertex(sh1);
            if (vertex.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");

            BRepAdaptor_Surface surf (face);
            Handle (Geom_Surface) hSurf = BRep_Tool::Surface(face);
            gp_Pnt p = BRep_Tool::Pnt(vertex);

            GeomAPI_ProjectPointOnSurf projector(p, hSurf);
            double u, v;
            if (projector.NbPoints()==0)
                throw Base::Exception("Part2DObject::positionBySupport: projecting point onto surface failed.");
            projector.Parameters(1, u, v);

            BRepLProp_SLProps prop(surf,u,v,1, Precision::Confusion());
            SketchNormal = prop.Normal().Reversed();

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
        case mmNormalToPathRev: {//all alignments to poing on curve
            const TopoDS_Edge &path = TopoDS::Edge(sh0);
            if (path.IsNull())
                throw Base::Exception("Null path in Part2DObject::positionBySupport()!");

            BRepAdaptor_Curve adapt(path);

            double u = 0.0;
            double u1 = adapt.FirstParameter();
            double u2 = adapt.LastParameter();

            //if a point is specified, use the point as a point of mapping, otherwise use parameter value from properties
            if (sub.size() >= 2) {
                TopoDS_Shape sh1;
                try {
                    sh1 = shape.getSubShape(sub[1].c_str());
                }
                catch (Standard_Failure) {
                    throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
                }
                TopoDS_Vertex vertex = TopoDS::Vertex(sh1);
                if (vertex.IsNull())
                    throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");

                gp_Pnt p = BRep_Tool::Pnt(vertex);

                Handle (Geom_Curve) hCurve = BRep_Tool::Curve(path, u1, u2);

                GeomAPI_ProjectPointOnCurve projector = GeomAPI_ProjectPointOnCurve (p, hCurve);
                projector.Parameter(1, u);
            } else {
                u = u1  +  this->MapPathParameter.getValue() * (u2 - u1);
            }
            gp_Pnt p;  gp_Vec d; //point and derivative
            adapt.D1(u,p,d);

            if (d.Magnitude()<Precision::Confusion())
                throw Base::Exception("Part2DObject::positionBySupport: path curve derivative is below 1e-7, too low, can't align");

            if (mmode == mmNormalToPathRev
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
                case mmNormalToPathRev:
                    SketchNormal = T.Reversed();//to avoid sketches upside-down for regular curves like circles
                    SketchXAxis = N.Reversed();
                    if (mmode == mmNormalToPathRev) {
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
        case mmThreePoints: {
            if (sub.size() < 3)
                throw Base::Exception("Part2DObject::positionBySupport: less than 3 subshapes specified for TangentPlane alignment mode.");
            TopoDS_Shape sh1;
            try {
                sh1 = shape.getSubShape(sub[1].c_str());
            }
            catch (Standard_Failure) {
                throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
            }
            TopoDS_Shape sh2;
            try {
                sh2 = shape.getSubShape(sub[2].c_str());
            }
            catch (Standard_Failure) {
                throw Base::Exception("Face/Edge/Vertex in support shape doesn't exist!");
            }

            const TopoDS_Vertex &vertex0 = TopoDS::Vertex(sh0);
            if (vertex0.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
            const TopoDS_Vertex &vertex1 = TopoDS::Vertex(sh1);
            if (vertex1.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");
            const TopoDS_Vertex &vertex2 = TopoDS::Vertex(sh2);
            if (vertex2.IsNull())
                throw Base::Exception("Null vertex in Part2DObject::positionBySupport()!");

            gp_Pnt p0 = BRep_Tool::Pnt(vertex0);
            gp_Pnt p1 = BRep_Tool::Pnt(vertex1);
            gp_Pnt p2 = BRep_Tool::Pnt(vertex2);

            gp_Vec tangent1 (p0,p1);
            gp_Vec tangent2 (p0,p2);
            gp_Vec norm = tangent1.Crossed(tangent2);
            if (norm.Magnitude() < Precision::SquareConfusion())
                throw Base::Exception("Part2DObject::positionBySupport: points are collinear. Can't make a plane");

            norm.Normalize();
            SketchNormal = gp_Dir(norm);

            gp_Pnt ObjOrg(Place.getPosition().x,Place.getPosition().y,Place.getPosition().z);

            Handle (Geom_Plane) gPlane = new Geom_Plane(p0,SketchNormal);
            GeomAPI_ProjectPointOnSurf projector(ObjOrg,gPlane);
            SketchBasePoint = projector.NearestPoint();
        } break;
        default:
            assert(0/*Attachment mode is not implemented?*/);
            Base::Console().Error("Attachment mode %s is not implemented.\n", int(this->MapMode.getValueAsString()));
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

Part2DObject::eMapMode Part2DObject::SuggestAutoMapMode(const App::PropertyLinkSub& Support) const
{
    Part::Feature *part = static_cast<Part::Feature*>(Support.getValue());
    if (!part || !part->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return mmDeactivated;

    const std::vector<std::string> &sub = Support.getSubValues();
    std::string typeList;
    for (int i=0; i<sub.size(); i++) {
        char c = (sub[i])[0];
        if (c == 0)
            c = '.';
        typeList.append(sub[i],c,1);
    }

    //F = face, E = edge, V = vertex
    if (typeList == "F") {
        return mmToFlatFace;
    } else if (typeList == "FV") {
        return mmTangentPlane;
    } else if (typeList == "E" || typeList == "EV") {
        return mmNormalToPath;
    } else if (typeList == "VVV") {
        return mmThreePoints;
    } else {
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
    double period = 0;
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
        if (fabs(param2-param1-period) < 1e-10) {
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

