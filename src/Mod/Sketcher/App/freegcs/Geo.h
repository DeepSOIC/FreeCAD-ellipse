/***************************************************************************
 *   Copyright (c) Konstantinos Poulios      (logari81@gmail.com) 2011     *
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

#ifndef FREEGCS_GEO_H
#define FREEGCS_GEO_H

#include <cmath>

namespace GCS
{

    ///////////////////////////////////////
    // Geometries
    ///////////////////////////////////////
    class Vector2D //DeepSOIC: I tried to reuse Base::Vector2D by #include <Base/Tools2D.h>, but failed to resolve buttshit compilation errors that arose in the process, so...
    {
    public:
        Vector2D(){x=0; y=0;}
        Vector2D(double x, double y) {this->x = x; this->y = y;}
        double x;
        double y;
        double length() {return sqrt(x*x + y*y);}
        Vector2D getNormalized(){double l=length(); if(l==0.0) l=1.0; return Vector2D(x/l,y/l);} //returns zero vector if the original is zero.
    };

    class Point
    {
    public:
        Point(){x = 0; y = 0;}
        double *x;
        double *y;
    };

    class Curve //a base class for all curve-based objects
    {
    public:
        //returns normal vector. The vector should point inward, indicating direction towards center of curvature.
        //derivparam is a pointer to a curve parameter to compute the derivative for. if derivparam is nullptr,
        //the actual normal vector is returned, otherwise a derivative of normal vector by *derivparam is returned
        virtual Vector2D CalculateNormal(Point &p, double* derivparam = nullptr) = 0;
    };

    class Line: public Curve
    {
    public:
        Line(){}
        Point p1;
        Point p2;
        Vector2D CalculateNormal(Point &p, double* derivparam = nullptr);
    };

    class Circle: public Curve
    {
    public:
        Circle(){rad = 0;}
        Point center;
        double *rad;
        Vector2D CalculateNormal(Point &p, double* derivparam = nullptr);
    };

    class Arc: public Circle
    {
    public:
        Arc(){startAngle=0;endAngle=0;rad=0;}
        double *startAngle;
        double *endAngle;
        //double *rad;
        Point start;
        Point end;
        //Point center;

    };
    
    class Ellipse: public Curve
    {
    public:
        Ellipse(){ radmin = 0;}
        Point center; 
        double *focus1X;
        double *focus1Y;
        double *radmin;
        Vector2D CalculateNormal(Point &p, double* derivparam = nullptr);
    };
    
    class ArcOfEllipse: public Ellipse
    {
    public:
        ArcOfEllipse(){startAngle=0;endAngle=0;radmin = 0;}
        double *startAngle;
        double *endAngle;
        //double *radmin;
        Point start;
        Point end;
        //Point center;
        //double *focus1X; //+u
        //double *focus1Y;
    };

} //namespace GCS

#endif // FREEGCS_GEO_H
