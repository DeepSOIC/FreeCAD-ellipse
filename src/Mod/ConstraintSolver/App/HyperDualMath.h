/***************************************************************************
 *   Copyright (c) 2019 Viktor Titov (DeepSOIC) <vv.titov@gmail.com>       *
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

#ifndef FREECAD_CONSTRAINTSOLVER_HYPERDUALMATH_H
#define FREECAD_CONSTRAINTSOLVER_HYPERDUALMATH_H

#include "HyperDualNumber.h"
#include "DualMath.h"
#include "cmath"

namespace FCS {

using DualNumber = Base::DualNumber;

//the overall idea here is to recursively evaluate on lower-grade hyperduals till the base is reached, and then DualMath implementation is finally used.
//The idea is the same as in cppduals library, but there, the downgraded version of x is stored fully as its real part (with all)
template <int D>
inline HyperDualNumber<D> sq(HyperDualNumber<D> x){
    return HyperDualNumber<D>(sq(x.re), 2 * x.downgraded() * x.du);
}

template <int D>
inline HyperDualNumber<D> sqrt(HyperDualNumber<D> x){
    auto subret = sqrt(x.downgraded());
    return HyperDualNumber<D>(subret.re, x.du * 0.5 / subret);
}

inline void _test1()
{
    (void)(sq(HyperDual3(3)));
    (void)(sqrt(HyperDual3(3)));
}

//inline HyperDualNumber<D> sin(HyperDualNumber<D> ang) {
//    return HyperDualNumber<D>(::sin(ang.re), ang.du * ::cos(ang.re));
//}
//
//inline HyperDualNumber<D> cos(HyperDualNumber<D> ang) {
//    return HyperDualNumber<D>(::cos(ang.re), -ang.du * ::sin(ang.re));
//}
//
//inline HyperDualNumber<D> sinh(HyperDualNumber<D> ang) {
//    return HyperDualNumber<D>(::sinh(ang.re), ang.du * ::cosh(ang.re));
//}
//
//inline HyperDualNumber<D> cosh(HyperDualNumber<D> ang) {
//    return HyperDualNumber<D>(::cosh(ang.re), ang.du * ::sinh(ang.re));
//}
//
//inline HyperDualNumber<D> atan2(HyperDualNumber<D> y, HyperDualNumber<D> x) {
//    double re = ::atan2(y.re, x.re);
//    double du = (x.du * -y.re + y.du * x.re)/(sq(x.re) + sq(y.re));
//    return HyperDualNumber<D>(re, du);
//}
//
/////atan2 assuming x^2+y^2 == 1 (slightly faster)
//inline HyperDualNumber<D> atan2n(HyperDualNumber<D> y, HyperDualNumber<D> x) {
//    double re = ::atan2(y.re, x.re);
//    double du = (x.du * -y.re + y.du * x.re);
//    return HyperDualNumber<D>(re, du);
//}
//
//inline HyperDualNumber<D> exp(HyperDualNumber<D> a){
//    double ret = ::exp(a.re);
//    return HyperDualNumber<D>(ret, a.du * ret);
//}
//
//inline HyperDualNumber<D> ln(HyperDualNumber<D> a){
//    return HyperDualNumber<D>(::log(a.re), a.du / a.re);
//}

/////returns an angle in [0..TURN) range
//inline HyperDualNumber<D> positiveAngle(HyperDualNumber<D> ang){
//    return ang - ::floor((ang.re + 1e-12) / TURN) * TURN;
//}
//
/////returns an angle in (-PI..PI] range
//inline HyperDualNumber<D> signedAngle(HyperDualNumber<D> ang){
//    return ang - ::floor(0.5 + (ang.re - 1e-12) / TURN) * TURN;
//}

template<int D>
inline HyperDualNumber<D> operator^(HyperDualNumber<D> a, bool reversed){
    return a * (reversed ? -1.0 : 1.0);
}
} //namespace


#endif
