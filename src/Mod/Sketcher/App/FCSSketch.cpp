/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <BRep_Builder.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS_Compound.hxx>
# include <Standard_Version.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <cmath>
# include <iostream>
#endif

#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/VectorPy.h>
#include <Base/StdStlTools.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/GeometryCurvePy.h>
#include <Mod/Part/App/ArcOfCirclePy.h>
#include <Mod/Part/App/ArcOfEllipsePy.h>
#include <Mod/Part/App/CirclePy.h>
#include <Mod/Part/App/EllipsePy.h>
#include <Mod/Part/App/HyperbolaPy.h>
#include <Mod/Part/App/ArcOfHyperbolaPy.h>
#include <Mod/Part/App/ParabolaPy.h>
#include <Mod/Part/App/ArcOfParabolaPy.h>
#include <Mod/Part/App/LineSegmentPy.h>
#include <Mod/Part/App/BSplineCurvePy.h>

#include <Mod/ConstraintSolver/App/ConstraintEqualShape.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintPointCoincident.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintDirectionalDistance.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintVertical.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintHorizontal.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintPointOnCurve.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintDistance.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintDistanceLinePoint.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintPointSymmetry.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintTangentLineLine.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintTangentCircleLine.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintTangentEllipseLine.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintTangentCircleCircle.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintAngleLineLine.h>
#include <Mod/ConstraintSolver/App/G2D/ConstraintAngleAtXY.h>

#include <Mod/ConstraintSolver/App/SubSystem.h>
#include <Mod/ConstraintSolver/App/LM.h>

#include "FCSSketch.h"
#include "Constraint.h"

using namespace Sketcher;
using namespace Base;
using namespace Part;

TYPESYSTEM_SOURCE(Sketcher::FCSSketch, Sketcher::SketchSolver)


FCSSketch::FCSSketch() : ParameterStore(Py::None())
                         ,malformedConstraints(false)
                         ,ConstraintsCounter(0)
{
    ParameterStore = FCS::ParameterStore::make();
}


void FCSSketch::clear(void)
{
    ParameterStore = FCS::ParameterStore::make(); // get a new parameterstore

    Geoms.clear();

    Points.clear();
    LineSegments.clear();
    Circles.clear();
    Ellipses.clear();
    Arcs.clear();
    ArcsOfEllipse.clear();
    ArcsOfHyperbola.clear();

    /*
    ArcsOfParabola.clear();
    BSplines.clear();*/

    Constrs.clear();

    Conflicting.clear();
    Redundant.clear();

    ConstraintsCounter = 0;

    //GCSsys.clear();
    //isInitMove = false;
}

int FCSSketch::setUpSketch(const std::vector<Part::Geometry *> &GeoList,
                        const std::vector<Constraint *> &ConstraintList,
                        int extGeoCount)
{

    Base::TimeInfo start_time;

    clear();

    std::vector<Part::Geometry *> intGeoList;
    std::vector<Part::Geometry *> extGeoList;

    std::vector<bool> blockedGeometry; // these geometries are blocked, frozen and sent as fixed parameters to the solver
    std::vector<bool> unenforceableConstraints; // these constraints are unenforceable due to a Blocked constraint

    getSolvableGeometryContraints(GeoList, ConstraintList, extGeoCount, intGeoList, extGeoList, blockedGeometry, unenforceableConstraints);

    addGeometry(intGeoList,blockedGeometry);

    addGeometry(extGeoList, true, true);

    // The Geoms list might be empty after an undo/redo
    if (!Geoms.empty()) {
        addConstraints(ConstraintList,unenforceableConstraints);
    }

    /*
    GCSsys.clearByTag(-1);
    GCSsys.declareUnknowns(Parameters);
    GCSsys.declareDrivenParams(DrivenParameters);
    GCSsys.initSolution(defaultSolverRedundant);
    GCSsys.getConflicting(Conflicting);
    GCSsys.getRedundant(Redundant);
    GCSsys.getDependentParams(pconstraintplistOut);

    calculateDependentParametersElements();

    if (debugMode==GCS::Minimal || debugMode==GCS::IterationLevel) {
        Base::TimeInfo end_time;

        Base::Console().Log("Sketcher::setUpSketch()-T:%s\n",Base::TimeInfo::diffTime(start_time,end_time).c_str());
    }

    return GCSsys.dofsNumber();
    */
    return 1; // otherwise it is fully constraint
}

int FCSSketch::addGeometry(const std::vector<Part::Geometry *> &geo,
                        const std::vector<bool> &blockedGeometry)
{
    assert(geo.size() == blockedGeometry.size());

    int ret = -1;
    std::vector<Part::Geometry *>::const_iterator it;
    std::vector<bool>::const_iterator bit;

    for ( it = geo.begin(), bit = blockedGeometry.begin(); it != geo.end() && bit !=blockedGeometry.end(); ++it, ++bit)
        ret = addGeometry(*it, *bit);
    return ret;
}

int FCSSketch::addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed, bool external)
{
    int ret = -1;
    for (std::vector<Part::Geometry *>::const_iterator it=geo.begin(); it != geo.end(); ++it)
        ret = addGeometry(*it, fixed, external);
    return ret;
}

int FCSSketch::addGeometry(const Part::Geometry *geo, bool fixed, bool external)
{

    if (geo->getTypeId() == GeomPoint::getClassTypeId()) { // add a point
        const GeomPoint *point = static_cast<const GeomPoint*>(geo);
        // create the definition struct for that geom
        if( point->Construction == false ) {
            return addPoint(*point, fixed, external);
        }
        else {
            return addPoint(*point, true, external);
        }
    } else if (geo->getTypeId() == GeomLineSegment::getClassTypeId()) { // add a line
        const GeomLineSegment *lineSeg = static_cast<const GeomLineSegment*>(geo);
        // create the definition struct for that geom
        return addLineSegment(*lineSeg, fixed, external);
    } else if (geo->getTypeId() == GeomCircle::getClassTypeId()) { // add a circle
        const GeomCircle *circle = static_cast<const GeomCircle*>(geo);
        // create the definition struct for that geom
        return addCircle(*circle, fixed, external);
    } else if (geo->getTypeId() == GeomEllipse::getClassTypeId()) { // add a ellipse
        const GeomEllipse *ellipse = static_cast<const GeomEllipse*>(geo);
        // create the definition struct for that geom
        return addEllipse(*ellipse, fixed, external);
    } else if (geo->getTypeId() == GeomArcOfCircle::getClassTypeId()) { // add an arc
        const GeomArcOfCircle *aoc = static_cast<const GeomArcOfCircle*>(geo);
        // create the definition struct for that geom
        return addArc(*aoc, fixed, external);
    } else if (geo->getTypeId() == GeomArcOfEllipse::getClassTypeId()) { // add an arc
        const GeomArcOfEllipse *aoe = static_cast<const GeomArcOfEllipse*>(geo);
        // create the definition struct for that geom
        return addArcOfEllipse(*aoe, fixed, external);
    } else if (geo->getTypeId() == GeomArcOfHyperbola::getClassTypeId()) { // add an arc of hyperbola
        const GeomArcOfHyperbola *aoh = static_cast<const GeomArcOfHyperbola*>(geo);
        // create the definition struct for that geom
        return addArcOfHyperbola(*aoh, fixed, external);
    } else if (geo->getTypeId() == GeomArcOfParabola::getClassTypeId()) { // add an arc of parabola
        const GeomArcOfParabola *aop = static_cast<const GeomArcOfParabola*>(geo);
        // create the definition struct for that geom
        return addArcOfParabola(*aop, fixed, external);
    } /* else if (geo->getTypeId() == GeomBSplineCurve::getClassTypeId()) { // add a bspline
        const GeomBSplineCurve *bsp = static_cast<const GeomBSplineCurve*>(geo);
        // create the definition struct for that geom
        return addBSpline(*bsp, fixed);
    }*/
    else {
        throw Base::TypeError("FCSSketch::addGeometry(): Unknown or unsupported type added to a sketch");
    }
}

void FCSSketch::initPoint(FCS::G2D::HParaPoint & hp, const Base::Vector3d & point, bool fixed, bool makeparameters)
{
    if(makeparameters)
        hp->makeParameters(ParameterStore);

    hp->x.savedValue() = point.x;
    hp->y.savedValue() = point.y;

    if(fixed) {
        hp->x.fix();
        hp->y.fix();
    }
}

void FCSSketch::initParam(FCS::ParameterRef &param, double value, bool fixed)
{
    param.savedValue() = value;

    if(fixed)
        param.fix();

}

int FCSSketch::addPoint(const Part::GeomPoint &point, bool fixed, bool external)
{
    // create our own copy
    GeomPoint *p = static_cast<GeomPoint*>(point.clone());

    // create solver elements
    Points.emplace_back((new FCS::G2D::ParaPoint())->getHandle<FCS::G2D::ParaPoint>());

    initPoint(Points.back(), p->getPoint(), fixed, /*makeparameters*/ true);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(p)));
    def.type = GeoType::Point;
    def.external = external;
    def.startPointId = Points.size() - 1;
    def.endPointId = Points.size() - 1;
    def.midPointId = Points.size() - 1;
    def.index = Points.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size() - 1;

}

int FCSSketch::addLineSegment(const Part::GeomLineSegment &lineSegment, bool fixed, bool external)
{
    // create our own copy
    GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>(lineSegment.clone());

    // get the points from the line
    Base::Vector3d start = lineSeg->getStartPoint();
    Base::Vector3d end   = lineSeg->getEndPoint();

    // create solver elements
    LineSegments.emplace_back((new FCS::G2D::ParaLine())->getHandle<FCS::G2D::ParaLine>());

    FCS::G2D::HParaLine & hl = LineSegments.back();
    hl->makeParameters(ParameterStore);

    initPoint(hl->p0, start, fixed);
    initPoint(hl->p1, end, fixed);

    Points.push_back(hl->p0);
    Points.push_back(hl->p1);

    // create the definition struct for that geom
    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(lineSeg)));
    def.type = GeoType::Line;
    def.external = external;
    def.startPointId = Points.size() - 2;
    def.endPointId = Points.size() - 1;
    def.index = LineSegments.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size() - 1;
}

int FCSSketch::addCircle(const Part::GeomCircle &cir, bool fixed, bool external)
{
    // create our own copy
    GeomCircle *circ = static_cast<GeomCircle*>(cir.clone());

    Base::Vector3d center = circ->getCenter();
    double radius         = circ->getRadius();

    Circles.emplace_back((new FCS::G2D::ParaCircle())->getHandle<FCS::G2D::ParaCircle>());

    FCS::G2D::HParaCircle & hc = Circles.back();
    hc->makeParameters(ParameterStore);

    initPoint(hc->center, center, fixed);

    initParam(hc->radius, radius, fixed);

    Points.push_back(hc->center);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(circ)));
    def.type = GeoType::Circle;
    def.external = external;
    def.midPointId = Points.size() - 1;
    def.index = Circles.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int FCSSketch::addEllipse(const Part::GeomEllipse &elip, bool fixed, bool external)
{
    // create our own copy
    GeomEllipse *elips = static_cast<GeomEllipse*>(elip.clone());

    Base::Vector3d center = elips->getCenter();
    double radmaj         = elips->getMajorRadius();
    double radmin         = elips->getMinorRadius();
    Base::Vector3d radmajdir = elips->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj*radmaj-radmin*radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F*radmajdir; //+x

    Ellipses.emplace_back(FCS::G2D::ParaEllipse::makeBare());

    FCS::G2D::HParaEllipse & he = Ellipses.back();
    he->makeParameters(ParameterStore);

    initPoint(he->center, center, fixed);

    initPoint(he->focus1, focus1, fixed);

    initParam(he->radmin, radmin, fixed);

    Points.push_back(he->center);
    Points.push_back(he->focus1);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(elips)));
    def.type = GeoType::Ellipse;
    def.external = external;
    def.midPointId = Points.size() - 2; // this takes midPointId+1
    def.index = Ellipses.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int FCSSketch::addArc(const Part::GeomArcOfCircle &arc, bool fixed, bool external)
{
    // create our own copy
    GeomArcOfCircle *arcc = static_cast<GeomArcOfCircle*>(arc.clone());

    Base::Vector3d center = arcc->getCenter();
    double radius         = arcc->getRadius();

    Base::Vector3d startPnt = arcc->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt   = arcc->getEndPoint(/*emulateCCW=*/true);

    Arcs.emplace_back(FCS::G2D::ParaCircle::makeArc());

    FCS::G2D::HParaCircle & harc = Arcs.back();
    harc->makeParameters(ParameterStore);

    initPoint(harc->center, center, fixed);
    initPoint(harc->p0, startPnt, fixed);
    initPoint(harc->p1, endPnt, fixed);
    initParam(harc->u0, arcc->getFirstParameter(), fixed);
    initParam(harc->u1, arcc->getLastParameter(), fixed);

    initParam(harc->radius, radius, fixed);

    Points.push_back(harc->p0);
    Points.push_back(harc->p1);
    Points.push_back(harc->center);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(arcc)));
    def.type = GeoType::Arc;
    def.external = external;
    def.startPointId = Points.size() - 3;
    def.endPointId = Points.size() - 2;
    def.midPointId = Points.size() - 1;
    def.index = Arcs.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int FCSSketch::addArcOfEllipse(const Part::GeomArcOfEllipse &arcellip, bool fixed, bool external)
{
    // create our own copy
    GeomArcOfEllipse *earc = static_cast<GeomArcOfEllipse*>(arcellip.clone());

    Base::Vector3d center       = earc->getCenter();
    Base::Vector3d startPnt     = earc->getStartPoint(/*emulateCCW=*/true);
    Base::Vector3d endPnt       = earc->getEndPoint(/*emulateCCW=*/true);
    double radmaj               = earc->getMajorRadius();
    double radmin               = earc->getMinorRadius();
    Base::Vector3d radmajdir    = earc->getMajorAxisDir();

    double dist_C_F = sqrt(radmaj*radmaj-radmin*radmin);
    // solver parameters
    Base::Vector3d focus1 = center + dist_C_F*radmajdir; //+x

    ArcsOfEllipse.emplace_back(FCS::G2D::ParaEllipse::makeBareArc());

    FCS::G2D::HParaEllipse & hearc = ArcsOfEllipse.back();
    hearc->makeParameters(ParameterStore);

    initPoint(hearc->center, center, fixed);
    initPoint(hearc->focus1, focus1, fixed);
    initPoint(hearc->p0, startPnt, fixed);
    initPoint(hearc->p1, endPnt, fixed);
    initParam(hearc->radmin, radmin, fixed);
    initParam(hearc->u0, earc->getFirstParameter(), fixed);
    initParam(hearc->u1, earc->getLastParameter(), fixed);

    Points.push_back(hearc->p0);
    Points.push_back(hearc->p1);
    Points.push_back(hearc->center);
    Points.push_back(hearc->focus1);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(earc)));
    def.type = GeoType::ArcOfEllipse;
    def.external = external;
    def.startPointId = Points.size() - 4;
    def.endPointId = Points.size() - 3;
    def.midPointId = Points.size() - 2;
    def.index = ArcsOfEllipse.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int FCSSketch::addArcOfHyperbola(const Part::GeomArcOfHyperbola &archyp, bool fixed, bool external)
{
    // create our own copy
    GeomArcOfHyperbola *aoh = static_cast<GeomArcOfHyperbola*>(archyp.clone());

    Base::Vector3d center   = aoh->getCenter();
    Base::Vector3d startPnt = aoh->getStartPoint();
    Base::Vector3d endPnt   = aoh->getEndPoint();
    double radmaj         = aoh->getMajorRadius();
    double radmin         = aoh->getMinorRadius();
    Base::Vector3d radmajdir = aoh->getMajorAxisDir();

    Base::Vector3d majaxispoint = center + radmaj * radmajdir;

    ArcsOfHyperbola.emplace_back(FCS::G2D::ParaHyperbola::makeBareArc());

    FCS::G2D::HParaHyperbola & haoh = ArcsOfHyperbola.back();
    haoh->makeParameters(ParameterStore);

    initPoint(haoh->center, center, fixed);
    initPoint(haoh->majorAxisPoint, majaxispoint, fixed);
    initPoint(haoh->p0, startPnt, fixed);
    initPoint(haoh->p1, endPnt, fixed);
    initParam(haoh->radmin, radmin, fixed);
    initParam(haoh->u0, aoh->getFirstParameter(), fixed);
    initParam(haoh->u1, aoh->getLastParameter(), fixed);

    Points.push_back(haoh->p0);
    Points.push_back(haoh->p1);
    Points.push_back(haoh->center);
    Points.push_back(haoh->majorAxisPoint);

    // create the definition struct for that geom
    Geoms.emplace_back(); // add new geometry
    GeoDef &def = Geoms.back();
    def.geo  = std::move(std::unique_ptr<Geometry>(static_cast<Geometry *>(aoh)));
    def.type = GeoType::ArcOfHyperbola;
    def.external = external;
    def.startPointId = Points.size() - 4;
    def.endPointId = Points.size() - 3;
    def.midPointId = Points.size() - 2;
    def.index = ArcsOfHyperbola.size() - 1;

    // return the position of the newly added geometry
    return Geoms.size()-1;
}

int FCSSketch::addArcOfParabola(const Part::GeomArcOfParabola &parabolaSegment, bool fixed, bool external)
{
    // Not yet implemented in FCS
    return -1;
}





// Constraints


int FCSSketch::addConstraint(const Constraint *constraint)
{
    if (Geoms.empty())
        throw Base::ValueError("Sketch::addConstraint. Can't add constraint to a sketch with no geometry!");
    int rtn = -1;

    Constrs.emplace_back();
    ConstrDef &c = Constrs.back();
    c.constr=const_cast<Constraint *>(constraint);
    c.driving=constraint->isDriving;

    switch (constraint->Type) {

    case DistanceX:
        if (constraint->FirstPos == none){ // horizontal length of a line

            auto &l = getParaLineHandle(constraint->First);

            rtn = addDistanceXConstraint(c, l->p0, l->p1);
        }
        else if (constraint->Second == Constraint::GeoUndef) {// point on fixed x-coordinate

            rtn = addCoordinateXConstraint(c, constraint->First,constraint->FirstPos);
        }
        else if (constraint->SecondPos != none) {// point to point horizontal distance

            auto &p0 = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &p1 = getParaPointHandle(constraint->Second,constraint->SecondPos);

            rtn = addDistanceXConstraint(c, p0, p1);
        }
        break;
    case DistanceY:
        if (constraint->FirstPos == none){ // vertical length of a line

            auto &l = getParaLineHandle(constraint->First);

            rtn = addDistanceYConstraint(c, l->p0, l->p1);
        }
        else if (constraint->Second == Constraint::GeoUndef){ // point on fixed y-coordinate

            rtn = addCoordinateYConstraint(c, constraint->First,constraint->FirstPos);
        }
        else if (constraint->SecondPos != none){ // point to point vertical distance

            auto &p0 = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &p1 = getParaPointHandle(constraint->Second,constraint->SecondPos);

            rtn = addDistanceYConstraint(c, p0, p1);
        }
        break;
    case Horizontal:
        if (constraint->Second == Constraint::GeoUndef) { // horizontal line

            auto &l = getParaLineHandle(constraint->First);

            rtn = addHorizontalConstraint(c, l->p0, l->p1);
        }
        else { // two points on the same horizontal line

            auto &p0 = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &p1 = getParaPointHandle(constraint->Second,constraint->SecondPos);

            rtn = addHorizontalConstraint(c, p0, p1);
        }
        break;
    case Vertical:
        if (constraint->Second == Constraint::GeoUndef) { // horizontal line

            auto &l = getParaLineHandle(constraint->First);

            rtn = addVerticalConstraint(c, l->p0, l->p1);
        }
        else { // two points on the same horizontal line

            auto &p0 = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &p1 = getParaPointHandle(constraint->Second,constraint->SecondPos);

            rtn = addVerticalConstraint(c, p0, p1);
        }
        break;
    case Coincident:
        rtn = addPointCoincidentConstraint(c,constraint->First,constraint->FirstPos,constraint->Second,constraint->SecondPos);
        break;
    case PointOnObject:
        rtn = addPointOnObjectConstraint(c, constraint->First,constraint->FirstPos, constraint->Second);
        break;
    case Parallel:
        rtn = addParallelConstraint(c, constraint->First,constraint->Second);
        break;
    /*
    case Perpendicular:
        if (constraint->FirstPos == none &&
                constraint->SecondPos == none &&
                constraint->Third == Constraint::GeoUndef){
            //simple perpendicularity
            rtn = addPerpendicularConstraint(constraint->First,constraint->Second);
        } else {
            //any other point-wise perpendicularity
            c.value = new double(constraint->getValue());
            if(c.driving)
                FixParameters.push_back(c.value);
            else {
                Parameters.push_back(c.value);
                DrivenParameters.push_back(c.value);
            }

            rtn = addAngleAtPointConstraint(
                        constraint->First, constraint->FirstPos,
                        constraint->Second, constraint->SecondPos,
                        constraint->Third, constraint->ThirdPos,
                        c.value, constraint->Type, c.driving);
        }
        break;
    */
    case Tangent:
        if (constraint->FirstPos == none &&
                constraint->SecondPos == none &&
                constraint->Third == Constraint::GeoUndef){
            //simple tangency
            rtn = addTangentConstraint(c, constraint->First,constraint->Second);
        } else {
            //any other point-wise tangency (endpoint-to-curve, endpoint-to-endpoint, tangent-via-point)
            rtn = addAngleAtPointConstraint(c, constraint->First, constraint->FirstPos,
                                               constraint->Second, constraint->SecondPos,
                                               constraint->Third, constraint->ThirdPos);
        }
        break;
    case Distance:
        if (constraint->SecondPos != none){ // point to point distance
            auto &p0 = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &p1 = getParaPointHandle(constraint->Second,constraint->SecondPos);

            rtn = addDistanceConstraint(c, p0, p1);
        }
        else if (constraint->Second != Constraint::GeoUndef) {
            auto &p = getParaPointHandle(constraint->First,constraint->FirstPos);

            auto &l = getParaLineHandle(constraint->Second);

            rtn = addDistanceConstraint(c, p, l);
        }
        else {// line length
            auto &l = getParaLineHandle(constraint->First);

            rtn = addDistanceConstraint(c, l->p0, l->p1);
        }
        break;
    case Angle:
        if (constraint->Third != Constraint::GeoUndef){

            addAngleAtPointConstraint(c, constraint->First, constraint->FirstPos,
                                         constraint->Second, constraint->SecondPos,
                                         constraint->Third, constraint->ThirdPos);

        } else if (constraint->SecondPos != none){ // angle between two lines (with explicit start points)

            auto &l1 = getParaLineHandle(constraint->First);

            auto &l2 = getParaLineHandle(constraint->Second);

            rtn = addAngleConstraint(c, l1, l2);

            /* This is implemented as line-line, without starting points.*/
        }
        else if (constraint->Second != Constraint::GeoUndef){ // angle between two lines

            auto &l1 = getParaLineHandle(constraint->First);

            auto &l2 = getParaLineHandle(constraint->Second);

            rtn = addAngleConstraint(c, l1, l2);

        }
        else if (constraint->First != Constraint::GeoUndef) {// orientation angle of a line
            auto &l1 = getParaLineHandle(constraint->First);

            auto &l2 = getAxis(Axes::HorizontalAxis);

            rtn = addAngleConstraint(c, l2, l1);
        }
        break;
    /* case Radius:
    {
        c.value = new double(constraint->getValue());
        if(c.driving)
            FixParameters.push_back(c.value);
        else {
            Parameters.push_back(c.value);
            DrivenParameters.push_back(c.value);
        }

        rtn = addRadiusConstraint(constraint->First, c.value,c.driving);
        break;
    }
    case Diameter:
    {
        c.value = new double(constraint->getValue());
        if(c.driving)
            FixParameters.push_back(c.value);
        else {
            Parameters.push_back(c.value);
            DrivenParameters.push_back(c.value);
        }

        rtn = addDiameterConstraint(constraint->First, c.value,c.driving);
        break;
    }*/
    case Equal:
        rtn = addEqualConstraint(c, constraint->First, constraint->Second);
        break;
    case Symmetric:
        if (constraint->ThirdPos != none)
            rtn = addSymmetricConstraint(c, constraint->First,constraint->FirstPos,
                                         constraint->Second,constraint->SecondPos,
                                         constraint->Third,constraint->ThirdPos);
        // TODO: Symmetry about a line not yet
        //       implemented
        //else
        //    rtn = addSymmetricConstraint(constraint->First,constraint->FirstPos,
        //                                 constraint->Second,constraint->SecondPos,constraint->Third);
        break;
    case InternalAlignment:
        switch(constraint->AlignmentType) {
            case EllipseMajorDiameter:
                rtn = addInternalAlignmentEllipseMajorDiameter(c, constraint->First, constraint->Second);
                break;
            case EllipseMinorDiameter:
                rtn = addInternalAlignmentEllipseMinorDiameter(c, constraint->First,constraint->Second);
                break;
            case EllipseFocus1:
                rtn = addInternalAlignmentEllipseFocus1(c, constraint->First,constraint->Second);
                break;
            case EllipseFocus2:
                rtn = addInternalAlignmentEllipseFocus2(c, constraint->First,constraint->Second);
                break;
            /*case HyperbolaMajor:
                rtn = addInternalAlignmentHyperbolaMajorDiameter(constraint->First,constraint->Second);
                break;
            case HyperbolaMinor:
                rtn = addInternalAlignmentHyperbolaMinorDiameter(constraint->First,constraint->Second);
                break;
            case HyperbolaFocus:
                rtn = addInternalAlignmentHyperbolaFocus(constraint->First,constraint->Second);
                break;
            case ParabolaFocus:
                rtn = addInternalAlignmentParabolaFocus(constraint->First,constraint->Second);
                break;
            case BSplineControlPoint:
                rtn = addInternalAlignmentBSplineControlPoint(constraint->First,constraint->Second, constraint->InternalAlignmentIndex);
                break;
            case BSplineKnotPoint:
                rtn = addInternalAlignmentKnotPoint(constraint->First,constraint->Second, constraint->InternalAlignmentIndex);
                break;*/
            default:
                break;
        }
        break;/*
    case SnellsLaw:
        {
            c.value = new double(constraint->getValue());
            c.secondvalue = new double(constraint->getValue());

            if(c.driving) {
                FixParameters.push_back(c.value);
                FixParameters.push_back(c.secondvalue);
            }
            else {
                Parameters.push_back(c.value);
                Parameters.push_back(c.secondvalue);
                DrivenParameters.push_back(c.value);
                DrivenParameters.push_back(c.secondvalue);

            }

            //assert(constraint->ThirdPos==none); //will work anyway...
            rtn = addSnellsLawConstraint(constraint->First, constraint->FirstPos,
                                         constraint->Second, constraint->SecondPos,
                                         constraint->Third,
                                         c.value, c.secondvalue,c.driving);
        }
        break;
    */
    case Sketcher::None: // ambiguous enum value
    case Sketcher::Block: // handled separately while adding geometry
    case NumConstraintTypes:
        break;
    }

    return rtn;
}

int FCSSketch::addConstraints(const std::vector<Constraint *> &ConstraintList)
{
    int rtn = -1;
    int cid = 0;

    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++cid) {
        rtn = addConstraint (*it);

        if(rtn == -1) {
            Base::Console().Error("Sketcher constraint number %d is malformed!\n",cid);
        }
    }

    return rtn;
}

int FCSSketch::addConstraints(const std::vector<Constraint *> &ConstraintList,
                           const std::vector<bool> &unenforceableConstraints)
{
    int rtn = -1;

    int cid = 0;
    for (std::vector<Constraint *>::const_iterator it = ConstraintList.begin();it!=ConstraintList.end();++it,++cid) {
        if (!unenforceableConstraints[cid] && (*it)->Type != Block && (*it)->isActive == true) {
            rtn = addConstraint (*it);

            if(rtn == -1) {
                Base::Console().Error("Sketcher constraint number %d is malformed!\n",cid);
            }
        }
        else {
            ++ConstraintsCounter; // For correct solver redundant reporting
        }
    }

    return rtn;
}

int FCSSketch::addPointCoincidentConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2, PointPos pos2)
{
    FCS::G2D::HParaPoint &p1 = getParaPointHandle(geoId1,pos1);
    FCS::G2D::HParaPoint &p2 = getParaPointHandle(geoId2,pos2);

    // TODO: FCS does not have tag - review the need
    // TODO: ToDShape: Empty shape transformation should be a single object

    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintPointCoincident(toDShape(p1),toDShape(p2));

    //GCSsys.addConstraintP2PCoincident(p1, p2, tag);
    // FCS.G2D.ConstraintPointCoincident
    // ConstraintPointCoincident(HShape_Point p1, HShape_Point p2, std::string label = "");

    return ConstraintsCounter;
}

int FCSSketch::addPointOnObjectConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2)
{
    auto &p = getParaPointHandle(geoId1,pos1);

    auto &crv = getParaCurveHandle(geoId2);

    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintPointOnCurve(toDShape(p),toDShape(crv));

    return ConstraintsCounter;
}

int FCSSketch::addParallelConstraint(ConstrDef &c, int geoId1, int geoId2)
{
    auto &l1 = getParaLineHandle(geoId1);
    auto &l2 = getParaLineHandle(geoId2);

    int tag = ++ConstraintsCounter;

    //c.fcsConstr = new FCS::G2D::ConstraintPointOnCurve(toDShape(p),toDShape(crv));

    return ConstraintsCounter;
}

int FCSSketch::addEqualConstraint(ConstrDef &c,int geoId1, int geoId2)
{
    auto &crv1 = getParaCurveHandle(geoId1);

    auto &crv2 = getParaCurveHandle(geoId2);

    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::ConstraintEqualShape(crv1, crv2);

    return ConstraintsCounter;
}


int FCSSketch::addCoordinateXConstraint(ConstrDef &c, int geoId, PointPos pos)
{
    geoId = getSketchIndex(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {

        FCS::G2D::HParaPoint &p = Points[pointId];

        int tag = ++ConstraintsCounter;

        /*c.fcsConstr = new FCS::G2D::ConstraintPointCoincident(toDShape(p1),toDShape(p2));

        addConstraintEqual(p.x, x, tagId, driving);

        c.fcsConstr = new FCS::G2D::ConstraintPointCoincident(toDShape(p1),toDShape(p2));
        GCSsys.addConstraintCoordinateX(p, value, tag, driving);*/

        return ConstraintsCounter;
    }
    return -1;
}

int FCSSketch::addCoordinateYConstraint(ConstrDef &c, int geoId, PointPos pos)
{
    geoId = getSketchIndex(geoId);

    int pointId = getPointId(geoId, pos);

    if (pointId >= 0 && pointId < int(Points.size())) {

        FCS::G2D::HParaPoint &p = Points[pointId];

        int tag = ++ConstraintsCounter;

        //GCSsys.addConstraintCoordinateY(p, value, tag, driving);

        return ConstraintsCounter;
    }
    return -1;
}

int FCSSketch::addDistanceXConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1)
{
    int tag = ++ConstraintsCounter;

    auto constr = FCS::G2D::ConstraintDirectionalDistance::makeConstraintHorizontalDistance(toDShape(p0),toDShape(p1));

    constr->makeParameters(ParameterStore);

    // TODO: Sketch differentiates Fixed from Movable from DrivenParameters - Here we consider fixed/movable - Review decision
    initParam(constr->dist, c.constr->getValue(), c.driving);

    c.fcsConstr = constr;

    return ConstraintsCounter;
}

int FCSSketch::addDistanceYConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1)
{
    int tag = ++ConstraintsCounter;

    auto constr = FCS::G2D::ConstraintDirectionalDistance::makeConstraintVerticalDistance(toDShape(p0),toDShape(p1));

    constr->makeParameters(ParameterStore);

    // TODO: Sketch differentiates Fixed from Movable from DrivenParameters - Here we consider fixed/movable - Review decision
    initParam(constr->dist, c.constr->getValue(), c.driving);

    c.fcsConstr = constr;

    return ConstraintsCounter;
}

int FCSSketch::addDistanceConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1)
{
    int tag = ++ConstraintsCounter;

    auto constr = new FCS::G2D::ConstraintDistance(toDShape(p0),toDShape(p1));

    constr->makeParameters(ParameterStore);

    initParam(constr->dist, c.constr->getValue(), c.driving);

    c.fcsConstr = constr;

    return ConstraintsCounter;
}

int FCSSketch::addDistanceConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p, FCS::G2D::HParaLine &l)
{
    int tag = ++ConstraintsCounter;

    auto constr = new FCS::G2D::ConstraintDistanceLinePoint(toDShape(l),toDShape(p));

    constr->makeParameters(ParameterStore);

    initParam(constr->dist, c.constr->getValue(), c.driving);

    c.fcsConstr = constr;

    return ConstraintsCounter;
}

int FCSSketch::addVerticalConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1)
{
    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintVertical(toDShape(p0),toDShape(p1));

    return ConstraintsCounter;

}

int FCSSketch::addHorizontalConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1)
{
    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintHorizontal(toDShape(p0),toDShape(p1));

    return ConstraintsCounter;

}

int FCSSketch::addInternalAlignmentEllipseMajorDiameter(ConstrDef &c, int geoId1, int geoId2)
{

    auto &e = getParaEllipseHandle(geoId2);

    auto &l = getParaLineHandle(geoId1);

    assert(e->majorDiameterLine.isNone());
    e->majorDiameterLine = l;

    return ConstraintsCounter;

}

int FCSSketch::addInternalAlignmentEllipseMinorDiameter(ConstrDef &c, int geoId1, int geoId2)
{
    auto &e = getParaEllipseHandle(geoId2);

    auto &l = getParaLineHandle(geoId1);

    assert(e->minorDiameterLine.isNone());
    e->minorDiameterLine = l;

    return ConstraintsCounter;
}

int FCSSketch::addInternalAlignmentEllipseFocus1(ConstrDef &c, int geoId1, int geoId2)
{
    auto &e = getParaEllipseHandle(geoId2);

    auto &p = getParaPointHandle(geoId1, start);

    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintPointCoincident(toDShape(p), toDShape(e->focus1));

    return ConstraintsCounter;
}

int FCSSketch::addInternalAlignmentEllipseFocus2(ConstrDef &c, int geoId1, int geoId2)
{
    auto &e = getParaEllipseHandle(geoId2);

    auto &p = getParaPointHandle(geoId1, start);

    //int tag = ++ConstraintsCounter;

    assert(e->focus2.isNone());
    e->focus2 = p;

    return ConstraintsCounter;
}



int FCSSketch::addSymmetricConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2, PointPos pos2,
                                      int geoId3, PointPos pos3)
{
    FCS::G2D::HParaPoint &p1 = getParaPointHandle(geoId1,pos1);
    FCS::G2D::HParaPoint &p2 = getParaPointHandle(geoId2,pos2);
    FCS::G2D::HParaPoint &p3 = getParaPointHandle(geoId3,pos3);

    int tag = ++ConstraintsCounter;

    c.fcsConstr = new FCS::G2D::ConstraintPointSymmetry(toDShape(p1),toDShape(p2),toDShape(p3));

    return ConstraintsCounter;
}

int FCSSketch::addTangentConstraint(ConstrDef &c, int geoId1, int geoId2)
{
    geoId1 = getSketchIndex(geoId1);
    geoId2 = getSketchIndex(geoId2);

    if (Geoms[geoId2].type == GeoType::Line) {
        if (Geoms[geoId1].type == GeoType::Line) {

            FCS::G2D::HParaLine &  hl1 = LineSegments[Geoms[geoId1].index];
            FCS::G2D::HParaLine &  hl2 = LineSegments[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentLineLine(toDShape(hl1),toDShape(hl2));

            return ConstraintsCounter;
        }
        else
            std::swap(geoId1, geoId2);
    }

    if (Geoms[geoId1].type == GeoType::Line) {
        FCS::G2D::HParaLine &  hl1 = LineSegments[Geoms[geoId1].index];

        if (Geoms[geoId2].type == GeoType::Arc) {
            auto & hc2 = Arcs[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleLine(toDShape(hc2),toDShape(hl1));

            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == GeoType::Circle) {
            auto & hc2 = Circles[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleLine(toDShape(hc2),toDShape(hl1));
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == GeoType::Ellipse) {
            auto & he2 = Ellipses[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentEllipseLine(toDShape(he2),toDShape(hl1));
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == GeoType::ArcOfEllipse) {
            auto & he2 = ArcsOfEllipse[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentEllipseLine(toDShape(he2),toDShape(hl1));
            return ConstraintsCounter;
        }
    } else if (Geoms[geoId1].type == GeoType::Circle) {
        auto &hc1 = Circles[Geoms[geoId1].index];

        if (Geoms[geoId2].type == GeoType::Circle) {
           auto & hc2 = Circles[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleCircle(toDShape(hc2),toDShape(hc1));
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == GeoType::Ellipse) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        }
        else if (Geoms[geoId2].type == GeoType::Arc) {
            auto & hc2 = Arcs[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleCircle(toDShape(hc2),toDShape(hc1));
            return ConstraintsCounter;
        }
    } else if (Geoms[geoId1].type == GeoType::Ellipse) {
        if (Geoms[geoId2].type == GeoType::Circle) {
            Base::Console().Error("Direct tangency constraint between circle and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        } else if (Geoms[geoId2].type == GeoType::Arc) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        }
    } else if (Geoms[geoId1].type == GeoType::Arc) {
        auto &hc1 = Arcs[Geoms[geoId1].index];
        if (Geoms[geoId2].type == GeoType::Circle) {
            auto & hc2 = Circles[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleCircle(toDShape(hc2),toDShape(hc1));
            return ConstraintsCounter;
        } else if (Geoms[geoId2].type == GeoType::Ellipse) {
            Base::Console().Error("Direct tangency constraint between arc and ellipse is not supported. Use tangent-via-point instead.");
            return -1;
        } else if (Geoms[geoId2].type == GeoType::Arc) {
            auto & hc2 = Arcs[Geoms[geoId2].index];

            int tag = ++ConstraintsCounter;

            c.fcsConstr = new FCS::G2D::ConstraintTangentCircleCircle(toDShape(hc2),toDShape(hc1));
            return ConstraintsCounter;
        }
    }

    return -1;
}

int FCSSketch::addAngleConstraint(ConstrDef &c, FCS::G2D::HParaLine &l1, FCS::G2D::HParaLine &l2)
{
    int tag = ++ConstraintsCounter;

    auto constr = new FCS::G2D::ConstraintAngleLineLine(toDShape(l1),toDShape(l2));

    constr->makeParameters(ParameterStore);

    initParam(constr->angle, c.constr->getValue(), c.driving);

    c.fcsConstr = constr;

    return ConstraintsCounter;
}

//This function handles any type of tangent, perpendicular and angle
// constraint that involves a point.
// i.e. endpoint-to-curve, endpoint-to-endpoint and tangent-via-point
//geoid1, geoid2 and geoid3 as in in the constraint object.
//For perp-ty and tangency, angle is used to lock the direction.
//angle==0 - autodetect direction. +pi/2, -pi/2 - specific direction.
int FCSSketch::addAngleAtPointConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2, PointPos pos2, int geoId3, PointPos pos3)
{

    if(!(c.constr->Type == Angle || c.constr->Type == Tangent || c.constr->Type == Perpendicular)) {
        //assert(0);//none of the three types. Why are we here??
        return -1;
    }

    bool avp = geoId3 != Constraint::GeoUndef; //is angle-via-point?
    bool e2c = pos2 == none  &&  pos1 != none;//is endpoint-to-curve?
    bool e2e = pos2 != none  &&  pos1 != none;//is endpoint-to-endpoint?

    if (!( avp || e2c || e2e )) {
        //assert(0);//none of the three types. Why are we here??
        return -1;
    }

    auto &crv1 = getParaCurveHandle(geoId1);
    auto &crv2 = getParaCurveHandle(geoId2);

    FCS::G2D::ConstraintAngleAtXY * cconstr;

    double anglevalue = c.constr->getValue();
    double angle = anglevalue;

    if (e2c || e2e) {
        //For tangency/perpendicularity, we don't just copy the angle.
        //The angle stored for tangency/perpendicularity is offset, so that the options
        // are -Pi/2 and Pi/2. If value is 0 - this is an indicator of an old sketch.
        // Use autodetect then.
        //The same functionality is implemented in SketchObject.cpp, where
        // it is used to permanently lock down the autodecision.
        if (c.constr->Type != Angle)
        {
            //The same functionality is implemented in SketchObject.cpp, where
            // it is used to permanently lock down the autodecision.
            double angleOffset = 0.0;//the difference between the datum value and the actual angle to apply. (datum=angle+offset)
            double angleDesire = 0.0;//the desired angle value (and we are to decide if 180* should be added to it)

            if (c.constr->Type == Tangent) {angleOffset = -M_PI/2; angleDesire = 0.0;}
            if (c.constr->Type == Perpendicular) {angleOffset = 0; angleDesire = M_PI/2;}

            if (angle==0.0) {//autodetect tangency internal/external (and same for perpendicularity)
                /*double angleErr = GCSsys.calculateAngleViaPoint(*crv1, *crv2, p) - angleDesire;
                //bring angleErr to -pi..pi
                if (angleErr > M_PI) angleErr -= M_PI*2;
                if (angleErr < -M_PI) angleErr += M_PI*2;

                //the autodetector
                if(fabs(angleErr) > M_PI/2 )
                    angleDesire += M_PI;
                */
                angle = angleDesire;
            } else {
                angle = anglevalue-angleOffset;
            }
        }


        auto &p1 = getParaPointHandle(geoId1, pos1); // e2e e2c p

        if(e2c) {
            // TODO: TAG -1
            c.auxConstrs.emplace_back((new FCS::G2D::ConstraintPointOnCurve(toDShape(p1),toDShape(crv2)))->getHandle<FCS::Constraint>());
        }
        else if (e2e){
            // TODO: TAG -1
            auto &p2 = getParaPointHandle(geoId2, pos2); // e2e p2

            c.auxConstrs.emplace_back((new FCS::G2D::ConstraintPointCoincident(toDShape(p1),toDShape(p2)))->getHandle<FCS::Constraint>());
        }

        ++ConstraintsCounter;
        cconstr = new FCS::G2D::ConstraintAngleAtXY(toDShape(crv1),toDShape(crv2), toDShape(p1));
    }
    else { // if (avp){

        auto &p3 = getParaPointHandle(geoId3, pos3); // avp p

        cconstr = new FCS::G2D::ConstraintAngleAtXY(toDShape(crv1),toDShape(crv2), toDShape(p3));
    }

    FCS::G2D::HConstraintAngleAtXY hcconstr = cconstr;

    hcconstr->makeParameters(ParameterStore);

    initParam(hcconstr->angle, angle, c.driving);

    c.fcsConstr = hcconstr;

    return ConstraintsCounter;
}



std::vector<Part::Geometry *> FCSSketch::extractGeometry(bool withConstructionElements,
                                                      bool withExternalElements) const
{
    std::vector<Part::Geometry *> temp;
    temp.reserve(Geoms.size());

    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it) {
        if ((!it->external || withExternalElements) && (!it->geo->Construction || withConstructionElements))
            temp.push_back(it->geo->clone());
    }

    return temp;
}


Base::Vector3d FCSSketch::calculateNormalAtPoint(int geoIdCurve, double px, double py)
{
    /*
    geoIdCurve = checkGeoId(geoIdCurve);

    GCS::Point p;
    p.x = &px;
    p.y = &py;

    //check pointers
    GCS::Curve* crv = getGCSCurveByGeoId(geoIdCurve);
    if (!crv) {
        throw Base::ValueError("calculateNormalAtPoint: getGCSCurveByGeoId returned NULL!\n");
    }

    double tx = 0.0, ty = 0.0;
    GCSsys.calculateNormalAtPoint(*crv, p, tx, ty);
    return Base::Vector3d(tx,ty,0.0);
    */
    return Base::Vector3d(1,1,1);
}

int FCSSketch::solve(void)
{
    if(Geoms.empty() || Constrs.empty())
        return 0;

    for(auto &c:Constrs) {
        if(!c.fcsConstr.isNone()) {
            c.fcsConstr->update();
        }

        for(auto &caux : c.auxConstrs) {
            if(!caux.isNone()) {
                caux->update();
            }
        }
    }

    FCS::HSubSystem sys = new FCS::SubSystem;

    FCS::HParameterSubset freesubset = FCS::ParameterSubset::make(ParameterStore->allFree());

    sys->addUnknown(freesubset);

    for(auto &c : Constrs) {
        if(!c.fcsConstr.isNone()) {
            sys->addConstraint(c.fcsConstr);
        }

        for(auto &caux : c.auxConstrs) {
            if(!caux.isNone()) {
                sys->addConstraint(caux);
            }
        }
    }

    for(auto &g : LineSegments)
        sys->addConstraint(g->makeRuleConstraints());

    for(auto &g : Arcs)
        sys->addConstraint(g->makeRuleConstraints());

    for(auto &g : ArcsOfEllipse)
        sys->addConstraint(g->makeRuleConstraints());

    for(auto &g : ArcsOfHyperbola)
        sys->addConstraint(g->makeRuleConstraints());

    // Posh Ellipses also have rules for the posh parameters
    for(auto &g : Ellipses){
        std::vector<FCS::HConstraint> cstrs = g->makeRuleConstraints();
        cstrs.back()->setReversed(true);//swap endpoints of diameter lines
        sys->addConstraint(cstrs);
    }

    FCS::HValueSet valueset = FCS::ValueSet::make(freesubset);

    FCS::HLM lmbackend = new FCS::LM;

    lmbackend->solve(sys,valueset);

    valueset->apply();

    updateGeometry();

    /*
    Base::TimeInfo start_time;
    if (!isInitMove) { // make sure we are in single subsystem mode
        GCSsys.clearByTag(-1);
        isFine = true;
    }

    int ret = -1;
    bool valid_solution;
    std::string solvername;
    int defaultsoltype = -1;

    if(isInitMove){
        solvername = "DogLeg"; // DogLeg is used for dragging (same as before)
        ret = GCSsys.solve(isFine, GCS::DogLeg);
    }
    else{
        switch (defaultSolver) {
            case 0:
                solvername = "BFGS";
                ret = GCSsys.solve(isFine, GCS::BFGS);
                defaultsoltype=2;
                break;
            case 1: // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                defaultsoltype=1;
                break;
            case 2: // solving with the BFGS solver
                solvername = "DogLeg";
                ret = GCSsys.solve(isFine, GCS::DogLeg);
                defaultsoltype=0;
                break;
        }
    }

    // if successfully solved try to write the parameters back
    if (ret == GCS::Success) {
        GCSsys.applySolution();
        valid_solution = updateGeometry();
        if (!valid_solution) {
            GCSsys.undoSolution();
            updateGeometry();
            Base::Console().Warning("Invalid solution from %s solver.\n", solvername.c_str());
        }
        else {
            updateNonDrivingConstraints();
        }
    }
    else {
        valid_solution = false;
        if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

            Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",solvername.c_str());
        }
    }

    if(!valid_solution && !isInitMove) { // Fall back to other solvers
        for (int soltype=0; soltype < 4; soltype++) {

            if(soltype==defaultsoltype){
                    continue; // skip default solver
            }

            switch (soltype) {
            case 0:
                solvername = "DogLeg";
                ret = GCSsys.solve(isFine, GCS::DogLeg);
                break;
            case 1: // solving with the LevenbergMarquardt solver
                solvername = "LevenbergMarquardt";
                ret = GCSsys.solve(isFine, GCS::LevenbergMarquardt);
                break;
            case 2: // solving with the BFGS solver
                solvername = "BFGS";
                ret = GCSsys.solve(isFine, GCS::BFGS);
                break;
            case 3: // last resort: augment the system with a second subsystem and use the SQP solver
                solvername = "SQP(augmented system)";
                InitParameters.resize(Parameters.size());
                int i=0;
                for (std::vector<double*>::iterator it = Parameters.begin(); it != Parameters.end(); ++it, i++) {
                    InitParameters[i] = **it;
                    GCSsys.addConstraintEqual(*it, &InitParameters[i], -1);
                }
                GCSsys.initSolution();
                ret = GCSsys.solve(isFine);
                break;
            }

            // if successfully solved try to write the parameters back
            if (ret == GCS::Success) {
                GCSsys.applySolution();
                valid_solution = updateGeometry();
                if (!valid_solution) {
                    GCSsys.undoSolution();
                    updateGeometry();
                    Base::Console().Warning("Invalid solution from %s solver.\n", solvername.c_str());
                    ret = GCS::SuccessfulSolutionInvalid;
                }else
                {
                    updateNonDrivingConstraints();
                }
            } else {
                valid_solution = false;
                if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

                    Base::Console().Log("Sketcher::Solve()-%s- Failed!! Falling back...\n",solvername.c_str());
                }
            }

            if (soltype == 3) // cleanup temporary constraints of the augmented system
                GCSsys.clearByTag(-1);

            if (valid_solution) {
                if (soltype == 1)
                    Base::Console().Log("Important: the LevenbergMarquardt solver succeeded where the DogLeg solver had failed.\n");
                else if (soltype == 2)
                    Base::Console().Log("Important: the BFGS solver succeeded where the DogLeg and LevenbergMarquardt solvers have failed.\n");
                else if (soltype == 3)
                    Base::Console().Log("Important: the SQP solver succeeded where all single subsystem solvers have failed.\n");

                if (soltype > 0) {
                    Base::Console().Log("If you see this message please report a way of reproducing this result at\n");
                    Base::Console().Log("http://www.freecadweb.org/tracker/main_page.php\n");
                }

                break;
            }
        } // soltype
    }

    Base::TimeInfo end_time;

    if(debugMode==GCS::Minimal || debugMode==GCS::IterationLevel){

        Base::Console().Log("Sketcher::Solve()-%s-T:%s\n",solvername.c_str(),Base::TimeInfo::diffTime(start_time,end_time).c_str());
    }

    SolveTime = Base::TimeInfo::diffTimeF(start_time,end_time);
    return ret;
    */
    return 0;
}

bool FCSSketch::updateGeometry()
{
    int i=0;
    for (std::vector<GeoDef>::const_iterator it=Geoms.begin(); it != Geoms.end(); ++it, i++) {
        try {
            if (it->type == GeoType::Point) {
                GeomPoint *point = static_cast<GeomPoint*>((*it).geo.get());

                if(!point->Construction) {
                    point->setPoint(Vector3d(Points[it->startPointId]->x.savedValue(),
                                         Points[it->startPointId]->y.savedValue(),
                                         0.0)
                               );
                }
            } else if (it->type == GeoType::Line) {
                GeomLineSegment *lineSeg = static_cast<GeomLineSegment*>((*it).geo.get());
                lineSeg->setPoints(Vector3d(LineSegments[it->index]->p0->x.savedValue(),
                                            LineSegments[it->index]->p0->y.savedValue(),
                                            0.0),
                                   Vector3d(LineSegments[it->index]->p1->x.savedValue(),
                                            LineSegments[it->index]->p1->y.savedValue(),
                                            0.0)
                                  );
            } else if (it->type == GeoType::Circle) {
                GeomCircle *circ = static_cast<GeomCircle*>((*it).geo.get());
                circ->setCenter(Vector3d(Points[it->midPointId]->x.savedValue(),
                                         Points[it->midPointId]->y.savedValue(),
                                         0.0)
                               );
                circ->setRadius(Circles[it->index]->radius.savedValue());
            } else if (it->type == GeoType::Ellipse) {

                GeomEllipse *ellipse = static_cast<GeomEllipse*>((*it).geo.get());

                Base::Vector3d center = Vector3d(Points[it->midPointId]->x.savedValue(),
                                         Points[it->midPointId]->y.savedValue(),
                                         0.0);
                Base::Vector3d f1 = Vector3d(Ellipses[it->index]->focus1->x.savedValue(),
                                             Ellipses[it->index]->focus1->y.savedValue(),
                                             0.0);
                double radmin = Ellipses[it->index]->radmin.savedValue();

                Base::Vector3d fd=f1-center;
                double radmaj = sqrt(fd*fd+radmin*radmin);

                ellipse->setCenter(center);
                if ( radmaj >= ellipse->getMinorRadius() ){//ensure that ellipse's major radius is always larger than minor raduis... may still cause problems with degenerates.
                    ellipse->setMajorRadius(radmaj);
                    ellipse->setMinorRadius(radmin);
                }  else {
                    ellipse->setMinorRadius(radmin);
                    ellipse->setMajorRadius(radmaj);
                }
                ellipse->setMajorAxisDir(fd);
            } else if (it->type == GeoType::Arc) {
                GeomArcOfCircle *aoc = static_cast<GeomArcOfCircle*>((*it).geo.get());
                aoc->setCenter(Vector3d(Points[it->midPointId]->x.savedValue(),
                                         Points[it->midPointId]->y.savedValue(),
                                         0.0)
                               );
                aoc->setRadius(Arcs[it->index]->radius.savedValue());
                aoc->setRange(Arcs[it->index]->u0.savedValue(), Arcs[it->index]->u1.savedValue(), true);
            } else if (it->type == GeoType::ArcOfEllipse) {
                GeomArcOfEllipse *aoe = static_cast<GeomArcOfEllipse*>((*it).geo.get());

                Base::Vector3d center = Vector3d(Points[it->midPointId]->x.savedValue(),
                                         Points[it->midPointId]->y.savedValue(),
                                         0.0);
                Base::Vector3d f1 = Vector3d(ArcsOfEllipse[it->index]->focus1->x.savedValue(),
                                             ArcsOfEllipse[it->index]->focus1->y.savedValue(),
                                             0.0);

                double radmin = ArcsOfEllipse[it->index]->radmin.savedValue();

                Base::Vector3d fd=f1-center;
                double radmaj = sqrt(fd*fd+radmin*radmin);

                aoe->setCenter(center);

                if ( radmaj >= aoe->getMinorRadius() ){//ensure that ellipse's major radius is always larger than minor raduis... may still cause problems with degenerates.
                    aoe->setMajorRadius(radmaj);
                    aoe->setMinorRadius(radmin);
                }  else {
                    aoe->setMinorRadius(radmin);
                    aoe->setMajorRadius(radmaj);
                }

                aoe->setMajorAxisDir(fd);
                aoe->setRange(ArcsOfEllipse[it->index]->u0.savedValue(), ArcsOfEllipse[it->index]->u1.savedValue(), true);
            } else if (it->type == GeoType::ArcOfHyperbola) {
                GeomArcOfHyperbola *aoh = static_cast<GeomArcOfHyperbola*>((*it).geo.get());

                Base::Vector3d center = Vector3d(Points[it->midPointId]->x.savedValue(),
                                         Points[it->midPointId]->y.savedValue(),
                                         0.0);

                Base::Vector3d radmajpoint = Vector3d(ArcsOfHyperbola[it->index]->majorAxisPoint->x.savedValue(),
                                             ArcsOfHyperbola[it->index]->majorAxisPoint->y.savedValue(),
                                             0.0);

                double radmin = ArcsOfHyperbola[it->index]->radmin.savedValue();

                Base::Vector3d fd = radmajpoint-center;
                double radmaj = (radmajpoint-center).Length();

                aoh->setCenter(center);

                if ( radmaj >= aoh->getMinorRadius() ){
                    aoh->setMajorRadius(radmaj);
                    aoh->setMinorRadius(radmin);
                }  else {
                    aoh->setMinorRadius(radmin);
                    aoh->setMajorRadius(radmaj);
                }

                aoh->setMajorAxisDir(fd);
                aoh->setRange(ArcsOfHyperbola[it->index]->u0.savedValue(), ArcsOfHyperbola[it->index]->u1.savedValue(), true);
            }/* else if (it->type == ArcOfParabola) {
                GCS::ArcOfParabola &myArc = ArcsOfParabola[it->index];

                GeomArcOfParabola *aop = static_cast<GeomArcOfParabola*>(it->geo);

                Base::Vector3d vertex = Vector3d(*Points[it->midPointId].x, *Points[it->midPointId].y, 0.0);
                Base::Vector3d f1 = Vector3d(*myArc.focus1.x, *myArc.focus1.y, 0.0);

                Base::Vector3d fd=f1-vertex;

                aop->setXAxisDir(fd);
                aop->setCenter(vertex);
                aop->setFocal(fd.Length());
                aop->setRange(*myArc.startAngle, *myArc.endAngle, emulateCCW=true);
            } else if (it->type == BSpline) {
                GCS::BSpline &mybsp = BSplines[it->index];

                GeomBSplineCurve *bsp = static_cast<GeomBSplineCurve*>(it->geo);

                std::vector<Base::Vector3d> poles;
                std::vector<double> weights;

                std::vector<GCS::Point>::const_iterator it1;
                std::vector<double *>::const_iterator it2;

                for( it1 = mybsp.poles.begin(), it2 = mybsp.weights.begin(); it1 != mybsp.poles.end() && it2 != mybsp.weights.end(); ++it1, ++it2) {
                    poles.emplace_back( *(*it1).x , *(*it1).y , 0.0);
                    weights.push_back(*(*it2));
                }

                bsp->setPoles(poles, weights);

                std::vector<double> knots;
                std::vector<int> mult;

                std::vector<double *>::const_iterator it3;
                std::vector<int>::const_iterator it4;

                for( it3 = mybsp.knots.begin(), it4 = mybsp.mult.begin(); it3 != mybsp.knots.end() && it4 != mybsp.mult.end(); ++it3, ++it4) {
                    knots.push_back(*(*it3));
                    mult.push_back((*it4));
                }

                bsp->setKnots(knots,mult);

                #if OCC_VERSION_HEX >= 0x060900
                int index = 0;
                for(std::vector<int>::const_iterator it5 = mybsp.knotpointGeoids.begin(); it5 != mybsp.knotpointGeoids.end(); ++it5, index++) {
                    if( *it5 != Constraint::GeoUndef) {
                        if (Geoms[*it5].type == Point) {
                            GeomPoint *point = static_cast<GeomPoint*>(Geoms[*it5].geo);

                            if(point->Construction) {
                                point->setPoint(bsp->pointAtParameter(knots[index]));
                            }
                        }
                    }
                }
                #endif

            }*/
        } catch (Base::Exception &e) {
            Base::Console().Error("Updating geometry: Error build geometry(%d): %s\n",
                                  i,e.what());
            return false;
        }
    }
    return true;
}



int FCSSketch::initMove(int geoId, PointPos pos, bool fine)
{
    /*
    isFine = fine;

    geoId = checkGeoId(geoId);

    GCSsys.clearByTag(-1);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts()) {
        isInitMove = false;
        return -1;
    }

    if (Geoms[geoId].type == Point) {
        if (pos == start) {
            GCS::Point &point = Points[Geoms[geoId].startPointId];
            GCS::Point p0;
            MoveParameters.resize(2); // px,py
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *point.x;
            *p0.y = *point.y;
            GCSsys.addConstraintP2PCoincident(p0,point,-1);
        }
    } else if (Geoms[geoId].type == Line) {
        if (pos == start || pos == end) {
            MoveParameters.resize(2); // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == start) {
                GCS::Point &p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == end) {
                GCS::Point &p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
        } else if (pos == none || pos == mid) {
            MoveParameters.resize(4); // x1,y1,x2,y2
            GCS::Point p1, p2;
            p1.x = &MoveParameters[0];
            p1.y = &MoveParameters[1];
            p2.x = &MoveParameters[2];
            p2.y = &MoveParameters[3];
            GCS::Line &l = Lines[Geoms[geoId].index];
            *p1.x = *l.p1.x;
            *p1.y = *l.p1.y;
            *p2.x = *l.p2.x;
            *p2.y = *l.p2.y;
            GCSsys.addConstraintP2PCoincident(p1,l.p1,-1);
            GCSsys.addConstraintP2PCoincident(p2,l.p2,-1);
        }
    } else if (Geoms[geoId].type == Circle) {
        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == none) {
            MoveParameters.resize(4); // x,y,cx,cy
            GCS::Circle &c = Circles[Geoms[geoId].index];
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y + *c.rad;
            GCSsys.addConstraintPointOnCircle(p0,c,-1);
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    } else if (Geoms[geoId].type == Ellipse) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        }
    } else if (Geoms[geoId].type == ArcOfEllipse) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);

        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }

            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    } else if (Geoms[geoId].type == ArcOfHyperbola) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);

        }
    } else if (Geoms[geoId].type == ArcOfParabola) {

        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid || pos == none) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;

            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end) {

            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                            : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;

                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;

            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);

        }
    } else if (Geoms[geoId].type == BSpline) {
        if (pos == start || pos == end) {
            MoveParameters.resize(2); // x,y
            GCS::Point p0;
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            if (pos == start) {
                GCS::Point &p = Points[Geoms[geoId].startPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == end) {
                GCS::Point &p = Points[Geoms[geoId].endPointId];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            }
        } else if (pos == none || pos == mid) {
            GCS::BSpline &bsp = BSplines[Geoms[geoId].index];
            MoveParameters.resize(bsp.poles.size()*2); // x0,y0,x1,y1,....xp,yp

            int mvindex = 0;
            for(std::vector<GCS::Point>::iterator it = bsp.poles.begin(); it != bsp.poles.end() ; it++, mvindex++) {
                GCS::Point p1;
                p1.x = &MoveParameters[mvindex];
                mvindex++;
                p1.y = &MoveParameters[mvindex];

                *p1.x = *(*it).x;
                *p1.y = *(*it).y;

                GCSsys.addConstraintP2PCoincident(p1,(*it),-1);
            }

        }
    } else if (Geoms[geoId].type == Arc) {
        GCS::Point &center = Points[Geoms[geoId].midPointId];
        GCS::Point p0,p1;
        if (pos == mid) {
            MoveParameters.resize(2); // cx,cy
            p0.x = &MoveParameters[0];
            p0.y = &MoveParameters[1];
            *p0.x = *center.x;
            *p0.y = *center.y;
            GCSsys.addConstraintP2PCoincident(p0,center,-1);
        } else if (pos == start || pos == end || pos == none) {
            MoveParameters.resize(4); // x,y,cx,cy
            if (pos == start || pos == end) {
                GCS::Point &p = (pos == start) ? Points[Geoms[geoId].startPointId]
                                               : Points[Geoms[geoId].endPointId];;
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *p.x;
                *p0.y = *p.y;
                GCSsys.addConstraintP2PCoincident(p0,p,-1);
            } else if (pos == none) {
                GCS::Arc &a = Arcs[Geoms[geoId].index];
                p0.x = &MoveParameters[0];
                p0.y = &MoveParameters[1];
                *p0.x = *center.x;
                *p0.y = *center.y + *a.rad;
                GCSsys.addConstraintPointOnArc(p0,a,-1);
            }
            p1.x = &MoveParameters[2];
            p1.y = &MoveParameters[3];
            *p1.x = *center.x;
            *p1.y = *center.y;
            int i=GCSsys.addConstraintP2PCoincident(p1,center,-1);
            GCSsys.rescaleConstraint(i-1, 0.01);
            GCSsys.rescaleConstraint(i, 0.01);
        }
    }
    InitParameters = MoveParameters;

    GCSsys.initSolution();
    isInitMove = true;
    */
    return 0;
}

void FCSSketch::resetInitMove()
{
    //isInitMove = false;
}

int FCSSketch::movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative)
{
    /*
    geoId = checkGeoId(geoId);

    // don't try to move sketches that contain conflicting constraints
    if (hasConflicts())
        return -1;

    if (!isInitMove) {
        initMove(geoId, pos);
        initToPoint = toPoint;
        moveStep = 0;
    }
    else {
        if(!relative && RecalculateInitialSolutionWhileMovingPoint) {
            if (moveStep == 0) {
                moveStep = (toPoint-initToPoint).Length();
            }
            else {
                if( (toPoint-initToPoint).Length() > 20*moveStep) { // I am getting too far away from the original solution so reinit the solution
                    initMove(geoId, pos);
                    initToPoint = toPoint;
                }
            }
        }
    }

    if (relative) {
        for (int i=0; i < int(MoveParameters.size()-1); i+=2) {
            MoveParameters[i] = InitParameters[i] + toPoint.x;
            MoveParameters[i+1] = InitParameters[i+1] + toPoint.y;
        }
    } else if (Geoms[geoId].type == Point) {
        if (pos == start) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Line) {
        if (pos == start || pos == end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        } else if (pos == none || pos == mid) {
            double dx = (InitParameters[2]-InitParameters[0])/2;
            double dy = (InitParameters[3]-InitParameters[1])/2;
            MoveParameters[0] = toPoint.x - dx;
            MoveParameters[1] = toPoint.y - dy;
            MoveParameters[2] = toPoint.x + dx;
            MoveParameters[3] = toPoint.y + dy;
        }
    } else if (Geoms[geoId].type == Circle) {
        if (pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Arc) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == Ellipse) {
        if (pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfEllipse) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfHyperbola) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == ArcOfParabola) {
        if (pos == start || pos == end || pos == mid || pos == none) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        }
    } else if (Geoms[geoId].type == BSpline) {
        if (pos == start || pos == end) {
            MoveParameters[0] = toPoint.x;
            MoveParameters[1] = toPoint.y;
        } else if (pos == none || pos == mid) {
            GCS::BSpline &bsp = BSplines[Geoms[geoId].index];

            double cx = 0, cy = 0; // geometric center
            for (int i=0; i < int(InitParameters.size()-1); i+=2) {
                cx += InitParameters[i];
                cy += InitParameters[i+1];
            }

            cx /= bsp.poles.size();
            cy /= bsp.poles.size();

            for (int i=0; i < int(MoveParameters.size()-1); i+=2) {

                MoveParameters[i]       = toPoint.x + InitParameters[i] - cx;
                MoveParameters[i+1]     = toPoint.y + InitParameters[i+1] - cy;
            }

        }
    }

    return solve();
    */
    return 0;
}

int FCSSketch::getPointId(int sketchgeoIndex, PointPos pos) const
{
    // TODO: This function relies on a geoid as defined for Sketch, no negative indices, no support for external geometry.
    if(!checkBoundaries(sketchgeoIndex))
        return -1;


    return getSketchPointIdFromSketchIndex(sketchgeoIndex,pos);
}

int FCSSketch::getSketchPointIdFromSketchIndex(int sketchgeoIndex, PointPos pos) const
{
    switch (pos) {
        case start:
            return Geoms[sketchgeoIndex].startPointId;
        case end:
            return Geoms[sketchgeoIndex].endPointId;
        case mid:
            return Geoms[sketchgeoIndex].midPointId;
        case none:
            break;
    }

    return -1;
}

bool FCSSketch::checkBoundaries(int sketchgeoIndex) const
{
    if (sketchgeoIndex < 0 || sketchgeoIndex >= (int)Geoms.size())
        return false;

    return true;
}

int FCSSketch::getSketchIndex(int geoId) const
{
    if (geoId < 0)
        geoId += Geoms.size();//convert negative external-geometry index to index into Geoms

    if(!checkBoundaries(geoId))
        throw Base::IndexError("FCSSketch::getSketchIndex. GeoId index out range.");

    return geoId;
}

int FCSSketch::getSketchPointId(int geoId, PointPos pos) const
{
    int pointid = getSketchPointIdFromSketchIndex(getSketchIndex(geoId), pos);

    // point index returned from Geoms not valid for Points array
    assert(pointid >= 0 && pointid < int(Points.size()));

    return pointid;
}


FCS::G2D::HParaPoint &FCSSketch::getParaPointHandle(int geoId, PointPos pos)
{
    return Points[getSketchPointId(geoId,pos)];
}


FCS::G2D::HParaLine &FCSSketch::getParaLineHandle(int geoId)
{
    geoId = getSketchIndex(geoId);

    if (Geoms[geoId].type != GeoType::Line)
        throw Base::TypeError("FCSSketch:: getParaLineHandle. GeoId is not a line.");

    return LineSegments[Geoms[geoId].index];
}

FCS::G2D::HParaCurve &FCSSketch::getParaCurveHandle(int geoId)
{
    geoId = getSketchIndex(geoId);

    if (Geoms[geoId].type == GeoType::Line) {
        return LineSegments[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }
    else if (Geoms[geoId].type == GeoType::Circle) {
        return Circles[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }
    else if (Geoms[geoId].type == GeoType::Ellipse) {
        return Ellipses[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }
    else if (Geoms[geoId].type == GeoType::Arc) {
        return Arcs[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }
    else if (Geoms[geoId].type == GeoType::ArcOfEllipse) {
        return ArcsOfEllipse[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }
    else if (Geoms[geoId].type == GeoType::ArcOfHyperbola) {
        return ArcsOfHyperbola[Geoms[geoId].index].upcast<FCS::G2D::ParaCurve>();
    }

    throw Base::TypeError("FCSSketch:: getParaCurveHandle. GeoId is not a supported curve.");

}

FCS::G2D::HParaEllipse &FCSSketch::getParaEllipseHandle(int geoId, bool includearcs)
{
    geoId = getSketchIndex(geoId);

    if (Geoms[geoId].type == GeoType::Ellipse)
        return Ellipses[Geoms[geoId].index];

    if (Geoms[geoId].type == GeoType::ArcOfEllipse && includearcs)
        return ArcsOfEllipse[Geoms[geoId].index];

    throw Base::TypeError("FCSSketch:: getParaEllipseHandle. GeoId is not an ellipse or arc of ellipse.");
}

FCS::G2D::HParaCircle &FCSSketch::getParaCircleHandle(int geoId, bool includearcs)
{
    geoId = getSketchIndex(geoId);

    if (Geoms[geoId].type == GeoType::Circle)
        return Circles[Geoms[geoId].index];

    if (Geoms[geoId].type == GeoType::Arc && includearcs)
        return Arcs[Geoms[geoId].index];

    throw Base::TypeError("FCSSketch:: getParaCircleHandle. GeoId is not an circle or arc of circle.");
}


Base::Vector3d FCSSketch::getPoint(int geoId, PointPos pos) const
{
    geoId = getSketchIndex(geoId);
    int pointId = getPointId(geoId, pos);
    if (pointId != -1)
        return Base::Vector3d(Points[pointId]->x.savedValue(), Points[pointId]->y.savedValue(), 0);

    return Base::Vector3d();
}

FCS::G2D::HParaLine &FCSSketch::getAxis(Axes axis)
{
    switch(axis) {
        case Axes::HorizontalAxis:
            return getParaLineHandle(-1);
        case Axes::VerticalAxis:
            return getParaLineHandle(-2);
        default:
            throw Base::NotImplementedError("FCSSketch:: getAxis(). Not implemented axis.");

    }

}




TopoShape FCSSketch::toShape(void) const
{

    TopoShape result;
    std::vector<GeoDef>::const_iterator it=Geoms.begin();

#if 0

    bool first = true;
    for (;it!=Geoms.end();++it) {
        if (!it->geo->Construction) {
            TopoDS_Shape sh = it->geo->toShape();
            if (first) {
                first = false;
                result.setShape(sh);
            } else {
                result.setShape(result.fuse(sh));
            }
        }
    }
    return result;
#else
    std::list<TopoDS_Edge> edge_list;
    std::list<TopoDS_Wire> wires;

    // collecting all (non constructive and non external) edges out of the sketch
    for (;it!=Geoms.end();++it) {
        if (!it->external && !it->geo->Construction && (it->type != GeoType::Point)) {
            edge_list.push_back(TopoDS::Edge(it->geo->toShape()));
        }
    }

    // FIXME: Use ShapeAnalysis_FreeBounds::ConnectEdgesToWires() as an alternative
    //
    // sort them together to wires
    while (edge_list.size() > 0) {
        BRepBuilderAPI_MakeWire mkWire;
        // add and erase first edge
        mkWire.Add(edge_list.front());
        edge_list.pop_front();

        TopoDS_Wire new_wire = mkWire.Wire(); // current new wire

        // try to connect each edge to the wire, the wire is complete if no more edges are connectible
        bool found = false;
        do {
            found = false;
            for (std::list<TopoDS_Edge>::iterator pE = edge_list.begin(); pE != edge_list.end(); ++pE) {
                mkWire.Add(*pE);
                if (mkWire.Error() != BRepBuilderAPI_DisconnectedWire) {
                    // edge added ==> remove it from list
                    found = true;
                    edge_list.erase(pE);
                    new_wire = mkWire.Wire();
                    break;
                }
            }
        }
        while (found);

        // Fix any topological issues of the wire
        ShapeFix_Wire aFix;
        aFix.SetPrecision(Precision::Confusion());
        aFix.Load(new_wire);
        aFix.FixReorder();
        aFix.FixConnected();
        aFix.FixClosed();
        wires.push_back(aFix.Wire());
    }

    if (wires.size() == 1)
        result = *wires.begin();
    else if (wires.size() > 1) {
        // FIXME: The right way here would be to determine the outer and inner wires and
        // generate a face with holes (inner wires have to be tagged REVERSE or INNER).
        // that's the only way to transport a somewhat more complex sketch...
        //result = *wires.begin();

        // I think a compound can be used as container because it is just a collection of
        // shapes and doesn't need too much information about the topology.
        // The actual knowledge how to create a prism from several wires should go to the Pad
        // feature (Werner).
        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);
        for (std::list<TopoDS_Wire>::iterator wt = wires.begin(); wt != wires.end(); ++wt)
            builder.Add(comp, *wt);
        result.setShape(comp);
    }
#endif

    return result;
}

float FCSSketch::getSolveTime()
{
    return 0.0f;
}

void FCSSketch::setRecalculateInitialSolutionWhileMovingPoint(bool on)
{

}

// Persistence implementer -------------------------------------------------

unsigned int FCSSketch::getMemSize(void) const
{
    return 0;
}

void FCSSketch::Save(Writer &) const
{

}

void FCSSketch::Restore(XMLReader &)
{

}

