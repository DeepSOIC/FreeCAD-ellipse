/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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

#include "Mod/Part/App/BodyBase.h"

// inclusion of the generated files (generated out of ItemPy.xml)
#include "BodyBasePy.h"
#include "BodyBasePy.cpp"

using namespace Part;

// returns a string which represents the object e.g. when printed in python
std::string BodyBasePy::representation(void) const
{
    return std::string("<BodyBase object>");
}


PyObject *BodyBasePy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int BodyBasePy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}

PyObject* BodyBasePy::addFeature(PyObject *args)
{
    PyObject* featurePy;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &featurePy))
        return 0;

    App::DocumentObject* feature = static_cast<App::DocumentObjectPy*>(featurePy)->getDocumentObjectPtr();

    BodyBase* body = this->getBodyBasePtr();

    try {
        body->addFeature(feature);
    } catch (Base::Exception& e) {
        PyErr_SetString(PyExc_SystemError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject* BodyBasePy::removeFeature(PyObject *args)
{
    PyObject* featurePy;
    if (!PyArg_ParseTuple(args, "O!", &(App::DocumentObjectPy::Type), &featurePy))
        return 0;

    App::DocumentObject* feature = static_cast<App::DocumentObjectPy*>(featurePy)->getDocumentObjectPtr();
    BodyBase* body = this->getBodyBasePtr();

    try {
        body->removeFeature(feature);
    } catch (Base::Exception& e) {
        PyErr_SetString(PyExc_SystemError, e.what());
        return 0;
    }

    Py_Return;
}

PyObject*  BodyBasePy::removeModelFromDocument(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;

    getBodyBasePtr()->removeModelFromDocument();
    Py_Return;
}




