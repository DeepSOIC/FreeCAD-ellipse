/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_FCSSKETCH_H
#define SKETCHER_FCSSKETCH_H

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/ConstraintSolver/App/ParameterStore.h>

#include <Mod/ConstraintSolver/App/G2D/ParaPoint.h>
#include <Mod/ConstraintSolver/App/G2D/ParaLine.h>
#include <Mod/ConstraintSolver/App/G2D/ParaCircle.h>
#include <Mod/ConstraintSolver/App/G2D/ParaEllipse.h>
#include <Mod/ConstraintSolver/App/G2D/ParaHyperbola.h>
#include <Mod/ConstraintSolver/App/G2D/ParaParabola.h>

#include "Constraint.h"

#include "SketchSolver.h"

namespace Sketcher
{

class SketcherExport FCSSketch : public SketchSolver
{
    TYPESYSTEM_HEADER();

public:
    FCSSketch();
    virtual ~FCSSketch() override = default;

    //delete copy constructor and assignment, to stop MSVC from attempting to create them due to _declspec(dllexport) aka SketcherExport
    // Geoms is a std::vector of a move-only type, so copy constructor and assignment cannot be generated.
    FCSSketch(FCSSketch& other) = delete;
    void operator=(FCSSketch& other) = delete;

    // from base class
    virtual unsigned int getMemSize(void) const override;
    virtual void Save(Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    // from SketchSolver

    /// solve the actual set up sketch
    virtual int solve(void) override;

    virtual int setUpSketch(const std::vector<Part::Geometry *> &GeoList, const std::vector<Constraint *> &ConstraintList,
                    int extGeoCount=0) override;

    /// return the actual geometry of the sketch a TopoShape
    virtual Part::TopoShape toShape(void) const override;

    /// returns the actual geometry
    virtual std::vector<Part::Geometry *> extractGeometry(bool withConstructionElements=true,
                                                  bool withExternalElements=false) const override;

    /// retrieves the index of a point
    /// TODO: GeoId is the Sketch's GeoId, not the SketchObject GeoId, both match for normal/construction geometry, but not for external geometry
    /// see checkGeoId(), this should be corrected as external modules should not rely on an index that is expected to be internal.
    virtual int getPointId(int sketchgeoIndex, PointPos pos) const override;

    /// retrieves a point
    virtual Base::Vector3d getPoint(int geoId, PointPos pos) const override;

    // Inline methods
    virtual inline bool hasConflicts(void) const override { return !Conflicting.empty(); }
    virtual inline const std::vector<int> &getConflicting(void) const override { return Conflicting; }
    virtual inline bool hasRedundancies(void) const override { return !Redundant.empty(); }
    virtual inline const std::vector<int> &getRedundant(void) const override { return Redundant; }

    inline virtual bool hasMalformedConstraints(void) const override { return malformedConstraints; }

    /** initializes a point (or curve) drag by setting the current
      * sketch status as a reference
      */
    virtual int initMove(int geoId, PointPos pos, bool fine=true) override;

    /** Resets the initialization of a point or curve drag
     */
    virtual void resetInitMove() override;

    /** move this point (or curve) to a new location and solve.
      * This will introduce some additional weak constraints expressing
      * a condition for satisfying the new point location!
      * The relative flag permits moving relatively to the current position
      */
    virtual int movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative=false) override;

    //This is to be used for rendering of angle-via-point constraint.
    virtual Base::Vector3d calculateNormalAtPoint(int geoIdCurve, double px, double py) override;

    //icstr should be the value returned by addXXXXConstraint
    //see more info in respective function in GCS.
    //double calculateConstraintError(int icstr) { return GCSsys.calculateConstraintErrorByTag(icstr);}

    /// Returns the size of the Geometry
    virtual int getGeometrySize(void) const override {return Geoms.size();}

    virtual float getSolveTime() override;
    virtual void setRecalculateInitialSolutionWhileMovingPoint(bool on) override;

private:

    enum class GeoType {
        None    = 0,
        Point   = 1, // 1 Point(start), 2 Parameters(x,y)
        Line    = 2, // 2 Points(start,end), 4 Parameters(x1,y1,x2,y2)
        Arc     = 3, // 3 Points(start,end,mid), (4)+5 Parameters((x1,y1,x2,y2),x,y,r,a1,a2)
        Circle  = 4, // 1 Point(mid), 3 Parameters(x,y,r)
        Ellipse = 5,  // 1 Point(mid), 5 Parameters(x,y,r1,r2,phi)  phi=angle xaxis of ellipse with respect of sketch xaxis
        ArcOfEllipse = 6,
        ArcOfHyperbola = 7,
        ArcOfParabola = 8,
        BSpline = 9
    };

    enum class Axes {
        HorizontalAxis = 0,
        VerticalAxis = 1
    };

    /// container element to store and work with the geometric elements of this sketch
    struct GeoDef {
        GeoDef() : geo(nullptr),type(GeoType::None),external(false),index(-1),
                   startPointId(-1),midPointId(-1),endPointId(-1) {}

        // Make explicit that it is a noexcept move-only type
        GeoDef(const GeoDef &) = delete;
        GeoDef & operator=(const GeoDef &) = delete;

        GeoDef(GeoDef &&) noexcept = default;
        GeoDef & operator=(GeoDef &&) noexcept = default;


        std::unique_ptr<Part::Geometry>     geo;            // pointer to the geometry
        GeoType                             type;           // type of the geometry
        bool                                external;       // flag for external geometries
        int                                 index;          // index in the corresponding storage vector (Lines, Arcs, Circles, ...)
        int                                 startPointId;   // index in Points of the start point of this geometry
        int                                 midPointId;     // index in Points of the start point of this geometry
        int                                 endPointId;     // index in Points of the end point of this geometry
    };

    struct ConstrDef {
        ConstrDef() : constr(0)
                    , driving(true)
                    , fcsConstr(Py::None()){}

        // Default copy/move constructor/assignment

        Constraint *                        constr;         // pointer to the constraint - borrowed resource - SketchObject has ownership
        bool                                driving;
        FCS::HConstraint                    fcsConstr;
        std::vector<FCS::HConstraint>       auxConstrs;
    };

private:
    /// add unspecified geometry, where each element's "fixed" status is given by the blockedGeometry array
    int addGeometry(const std::vector<Part::Geometry *> &geo,
                    const std::vector<bool> &blockedGeometry);

    int addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed=false, bool external=false);

    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo, bool fixed=false, bool external=false);

    int addPoint(const Part::GeomPoint &point, bool fixed=false, bool external=false);
    int addLineSegment(const Part::GeomLineSegment &lineSegment, bool fixed=false, bool external=false);
    int addCircle(const Part::GeomCircle &cir, bool fixed=false, bool external=false);
    int addEllipse(const Part::GeomEllipse &elip, bool fixed=false, bool external=false);
    int addArc(const Part::GeomArcOfCircle &arc, bool fixed=false, bool external=false);
    int addArcOfEllipse(const Part::GeomArcOfEllipse &elip, bool fixed=false, bool external=false);
    int addArcOfHyperbola(const Part::GeomArcOfHyperbola &archyp, bool fixed=false, bool external=false);
    int addArcOfParabola(const Part::GeomArcOfParabola &parabolaSegment, bool fixed=false, bool external=false);

    void initPoint(FCS::G2D::HParaPoint & hp, const Base::Vector3d & point, bool fixed=false, bool makeparameters=false);
    void initParam(FCS::ParameterRef &param, double value, bool fixed=false);

    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint *> &ConstraintList);
    /// add all constraints in the list, provided that are enforceable
    int addConstraints(const std::vector<Constraint *> &ConstraintList,
                       const std::vector<bool> & unenforceableConstraints);
    /// add one constraint to the sketch
    int addConstraint(const Constraint *constraint);

    /// add a coincident constraint to two points of two geometries
    int addPointCoincidentConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2, PointPos pos2);

    int addPointOnObjectConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2);

    int addParallelConstraint(ConstrDef &c, int geoId1, int geoId2);

    int addEqualConstraint(ConstrDef &c, int geoId1, int geoId2);

    /**
    *   add a fixed X coordinate constraint to a point
    *
    *   double * value is a pointer to double allocated in the heap, containing the
    *   constraint value and already inserted into either the FixParameters or
    *   Parameters array, as the case may be.
    */
    int addCoordinateXConstraint(ConstrDef &c, int geoId, PointPos pos);
    /**
    *   add a fixed Y coordinate constraint to a point
    *
    *   double * value is a pointer to double allocated in the heap, containing the
    *   constraint value and already inserted into either the FixParameters or
    *   Parameters array, as the case may be.
    */
    int addCoordinateYConstraint(ConstrDef &c, int geoId, PointPos pos);


    int addDistanceXConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1);

    int addDistanceYConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1);

    int addDistanceConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1);

    int addDistanceConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p, FCS::G2D::HParaLine &l);

    int addHorizontalConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1);

    int addVerticalConstraint(ConstrDef &c, FCS::G2D::HParaPoint &p0, FCS::G2D::HParaPoint &p1);

    int addAngleConstraint(ConstrDef &c, FCS::G2D::HParaLine &l1, FCS::G2D::HParaLine &l2);

    int addAngleAtPointConstraint(ConstrDef &c, int geoId1, PointPos pos1,int geoId2, PointPos pos2, int geoId3, PointPos pos3);

    int addInternalAlignmentEllipseMajorDiameter(ConstrDef &c, int geoId1, int geoId2);

    int addInternalAlignmentEllipseMinorDiameter(ConstrDef &c, int geoId1, int geoId2);

    int addInternalAlignmentEllipseFocus1(ConstrDef &c, int geoId1, int geoId2);

    int addInternalAlignmentEllipseFocus2(ConstrDef &c, int geoId1, int geoId2);

    int addSymmetricConstraint(ConstrDef &c, int geoId1, PointPos pos1, int geoId2, PointPos pos2, int geoId3, PointPos pos3);

    int addTangentConstraint(ConstrDef &c, int geoId1, int geoId2);

    int getSketchIndex(int geoId) const;

    bool checkBoundaries(int sketchgeoIndex) const;

    int getSketchPointIdFromSketchIndex(int sketchgeoIndex, PointPos pos) const;

    int getSketchPointId(int geoId, PointPos pos) const;

    FCS::G2D::HParaPoint &getParaPointHandle(int geoId, PointPos pos);

    FCS::G2D::HParaLine &getParaLineHandle(int geoId);

    FCS::G2D::HParaCurve &getParaCurveHandle(int geoId);

    FCS::G2D::HParaEllipse &getParaEllipseHandle(int geoId, bool includearcs=true);

    FCS::G2D::HParaCircle &getParaCircleHandle(int geoId, bool includearcs=true);

    FCS::G2D::HParaLine &getAxis(Axes axis);

    void clear(void);

    bool updateGeometry(void);

private:
    // Solver
    FCS::HParameterStore ParameterStore;

    // Interface classes
    std::vector<GeoDef>                         Geoms;
    std::vector<FCS::G2D::HParaPoint>           Points;
    std::vector<FCS::G2D::HParaLine>            LineSegments;
    std::vector<FCS::G2D::HParaCircle>          Circles;
    std::vector<FCS::G2D::HParaEllipse>         Ellipses;
    std::vector<FCS::G2D::HParaCircle>          Arcs;
    std::vector<FCS::G2D::HParaEllipse>         ArcsOfEllipse;
    std::vector<FCS::G2D::HParaHyperbola>       ArcsOfHyperbola;
    //std::vector<FCS::G2D::HParaParabola>        ArcsOfParabola;

    std::vector<ConstrDef>                      Constrs;

    // Equation system diagnosis
    std::vector<int> Conflicting;
    std::vector<int> Redundant;

    bool malformedConstraints;


    int ConstraintsCounter;
};

} //namespace Sketcher

#endif // SKETCHER_FCSSKETCH_H
