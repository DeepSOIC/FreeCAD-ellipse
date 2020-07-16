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

#ifndef FREECAD_FCS_HYPERDUAL_NUMBER_T_H
#define FREECAD_FCS_HYPERDUAL_NUMBER_T_H

//temporary, to make qt creator clang code model happy
    #ifndef FCSExport
        #define FCSExport
    #endif
    #ifndef BaseExport
        #define BaseExport
    #endif

#include <Base/DualNumber.h>

namespace FCS {

using Base::DualNumber;

/** HyperDualNumber<HYPERDEPTH>: dual numbers for computing HYPERDEPTH derivatives.
 * All components are Base::DualNumbers, to allow first derivatives by solver parameters under the hood.
 */
template <int HYPERDEPTH>
class FCSExport HyperDualNumber
{
public:
    typedef HyperDualNumber<HYPERDEPTH-1> DualType;
public:
    DualNumber re;
    DualType du;
public:
    HyperDualNumber(){}
    HyperDualNumber(DualNumber re) {this->re = re;}
    HyperDualNumber(DualNumber re, DualType du)
        :re(re), du(du) {}

    DualType downgraded() const {return DualType(re, du.downgraded());}

    HyperDualNumber operator-() const {return HyperDualNumber(-re, -du);}

    PyObject* getPyObject() const;
    std::string repr() const;
};

template <>
class FCSExport HyperDualNumber<1>
{
public:
    typedef DualNumber DualType;
public:
    DualNumber re;
    DualType du;
public:
    HyperDualNumber(){}
    HyperDualNumber(DualNumber re) {this->re = re;}
    HyperDualNumber(DualNumber re, DualType du)
        :re(re), du(du) {}

    ///returns this number with one less derivative
    DualType downgraded() const {return DualType(re);}

    HyperDualNumber operator-() const {return HyperDualNumber(-re, -du);}

    PyObject* getPyObject() const;
    std::string repr() const;
};

template <int D>
inline HyperDualNumber<D> operator+(HyperDualNumber<D> a, HyperDualNumber<D> b) {
    return HyperDualNumber<D>(a.re + b.re, a.du + b.du);
}
template <int D>
inline HyperDualNumber<D> operator+(HyperDualNumber<D> a, DualNumber b) {
    return HyperDualNumber<D>(a.re + b, a.du);
}
template <int D>
inline HyperDualNumber<D> operator+(DualNumber b, HyperDualNumber<D> a) {
    return HyperDualNumber<D>(a.re + b, a.du);
}

template <int D>
inline HyperDualNumber<D> operator-(HyperDualNumber<D> a, HyperDualNumber<D> b) {
    return HyperDualNumber<D>(a.re - b.re, a.du - b.du);
}
template <int D>
inline HyperDualNumber<D> operator-(HyperDualNumber<D> a, DualNumber b) {
    return HyperDualNumber<D>(a.re - b, a.du);
}
template <int D>
inline HyperDualNumber<D> operator-(DualNumber b, HyperDualNumber<D> a) {
    return HyperDualNumber<D>(b - a.re, -a.du);
}

template <int D>
inline HyperDualNumber<D> operator*(HyperDualNumber<D> a, HyperDualNumber<D> b) {
    return HyperDualNumber<D>(a.re * b.re, a.downgraded() * b.du + a.du * b.downgraded());
}
template <int D>
inline HyperDualNumber<D> operator*(HyperDualNumber<D> a, DualNumber b) {
    return HyperDualNumber<D>(a.re * b, a.du * b);
}
template <int D>
inline HyperDualNumber<D> operator*(DualNumber b, HyperDualNumber<D> a) {
    return HyperDualNumber<D>(a.re * b, a.du * b);
}

template <int D>
inline HyperDualNumber<D> operator/(HyperDualNumber<D> a, HyperDualNumber<D> b) {
    return HyperDualNumber<D>(
        a.re / b.re,
        (a.du * b.downgraded() - a.downgraded() * b.du)  /  (b.downgraded() * b.downgraded())
    );
}
template <int D>
inline HyperDualNumber<D> operator/(DualNumber a, HyperDualNumber<D> b) {
    return HyperDualNumber<D>(
        a.re / b.re,
        (- a * b.du)  /  (b.downgraded() * b.downgraded())
    );
}
template <int D>
inline HyperDualNumber<D> operator/(HyperDualNumber<D> a, DualNumber b) {
    return HyperDualNumber<D>(
        a.re / b,
        a.du / b
    );
}

using HyperDual1 = HyperDualNumber<1>;
using HyperDual2 = HyperDualNumber<2>;
using HyperDual3 = HyperDualNumber<3>;

#define IMPLEMENT_HyperDualNumber_OPERATOR(op)              \
template <int D>\
inline bool operator op (HyperDualNumber<D> a, HyperDualNumber<D> b) {  \
    return a.re op b.re;                               \
}                                                      \
template <int D>\
inline bool operator op (HyperDualNumber<D> a, DualNumber b) {      \
    return a.re op b;                                  \
}                                                      \

IMPLEMENT_HyperDualNumber_OPERATOR(<);
IMPLEMENT_HyperDualNumber_OPERATOR(<=);
IMPLEMENT_HyperDualNumber_OPERATOR(>);
IMPLEMENT_HyperDualNumber_OPERATOR(>=);

template <int D>
inline HyperDualNumber<D> abs(HyperDualNumber<D> a) {
    return a < 0 ? -a : a;
}


//compile tests!
inline void _test()
{
    (void)(HyperDual3(0) + HyperDual3(1));
    (void)(HyperDual3(0) - HyperDual3(1));
    (void)(HyperDual3(0) * HyperDual3(1));
    (void)(HyperDual3(0) / HyperDual3(1));

    (void)(HyperDual3(0) + 1);
    (void)(1 - HyperDual3(1));
    (void)(HyperDual3(0) * 1);
    (void)(HyperDual3(0) / 1);
}


} //namespace


#endif
