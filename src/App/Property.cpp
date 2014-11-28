/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#	include <cassert>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Property.h"
#include "PropertyContainer.h"
#include "Expression.h"
#include <algorithm>
#include <boost/tokenizer.hpp>

using namespace App;


//**************************************************************************
//**************************************************************************
// Property
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::Property , Base::Persistence);

//**************************************************************************
// Construction/Destruction

// here the implemataion! description should take place in the header file!
Property::Property()
  :father(0)
{

}

Property::~Property()
{

}

const char* Property::getName(void) const
{
    return father->getPropertyName(this);
}

short Property::getType(void) const
{
    return father->getPropertyType(this);
}

const char* Property::getGroup(void) const
{
    return father->getPropertyGroup(this);
}

const char* Property::getDocumentation(void) const
{
    return father->getPropertyDocumentation(this);
}

void Property::setContainer(PropertyContainer *Father)
{
    father = Father;
}

void Property::touch()
{
    if (father)
        father->onChanged(this);
    StatusBits.set(0);
}

bool Property::isTouched() const
{
    if (StatusBits.test(0))
        return true;

    const std::vector<PropertyDependencyLink> & links = getDependencies().getLinks();
    std::vector<PropertyDependencyLink>::const_iterator It = links.begin();

    while (It != links.end()) {
        if ((*It).getProperty() != this && (*It).getProperty()->isTouched())
            return true;

        ++It;
    }

    return false;
}

void Property::hasSetValue(void)
{
    if (father)
        father->onChanged(this);
    StatusBits.set(0);
}

void Property::aboutToSetValue(void)
{
    if (father)
        father->onBeforeChange(this);
}

Property *Property::Copy(void) const 
{
    // have to be reimplemented by a subclass!
    assert(0);
    return 0;
}

void Property::Paste(const Property& /*from*/)
{
    // have to be reimplemented by a subclass!
    assert(0);
}

std::string Property::encodeAttribute(const std::string& str) const
{
    std::string tmp;
    for (std::string::const_iterator it = str.begin(); it != str.end(); ++it) {
        if (*it == '<')
            tmp += "&lt;";
        else if (*it == '"')
            tmp += "&quot;";
        else if (*it == '&')
            tmp += "&amp;";
        else if (*it == '>')
            tmp += "&gt;";
        else if (*it == '\r')
            tmp += "&#xD;";
        else if (*it == '\n')
            tmp += "&#xA;";
        else
            tmp += *it;
    }

    return tmp;
}

//**************************************************************************
//**************************************************************************
// PropertyLists
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TYPESYSTEM_SOURCE_ABSTRACT(App::PropertyLists , App::Property);


PropertyDependencyLink::PropertyDependencyLink(const Path &_path, const Property *_prop, void * _owner)
    : path(_path)
    , prop(_prop)
    , owner(_owner)
{
}

PropertyDependencyLink &PropertyDependencyLink::operator =(const PropertyDependencyLink &other)
{
    path = other.path;
    prop = other.prop;
    owner = other.owner;

    return *this;
}

Path::Path(const std::string & property)
{
    if (property.size() > 0)
        addComponent(Component::SimpleComponent(property));
}

bool Path::operator ==(const Path &other) const
{
    if (documentObjectName != other.documentObjectName)
        return false;
    if (components != other.components)
        return false;
    return true;
}

bool Path::operator <(const Path &other) const
{
    if (documentObjectName < other.documentObjectName)
        return true;

    if (documentObjectName > other.documentObjectName)
        return false;

    if (components.size() < other.components.size())
        return true;

    if (components.size() > other.components.size())
        return false;

    for (int i = 0; i < components.size(); ++i) {
        if (components[i].component < other.components[i].component)
            return true;
        if (components[i].component > other.components[i].component)
            return false;
        if (components[i].type < other.components[i].type)
            return true;
        if (components[i].type > other.components[i].type)
            return false;
        if (components[i].type == Component::ARRAY) {
            if (components[i].index < other.components[i].index)
                return true;
            if (components[i].index > other.components[i].index)
                return false;
        }
        else if (components[i].type == Component::MAP) {
            if (components[i].key < other.components[i].key)
                return true;
            if (components[i].key > other.components[i].key)
                return false;
        }
    }
    return false;
}

int Path::numComponents() const
{
    return components.size();
}

Path Path::parse(const char *expr)
{
    return ExpressionParser::parsePath(0, expr);
}

std::string Path::toString() const
{
    std::stringstream s;

    if (documentObjectName.size() > 0)
        s << documentObjectName << ":";

    s << getPropertyName() << getSubPathStr();

    return s.str();
}

std::string Path::getSubPathStr() const
{
    std::stringstream s;

    std::vector<Component>::const_iterator i = components.begin() + 1;
    while (i != components.end()) {
        s << "." << i->component;
        switch (i->type) {
        case Component::SIMPLE:
            break;
        case Component::MAP:
            s << "{" << i->key << "}";
            break;
        case Component::ARRAY:
            s << "[" << i->index << "]";
            break;
        default:
            assert(0);
        }

        ++i;
    }

    return s.str();
}


void PropertyDependencyLinkList::addDependency(const PropertyDependencyLink &dep)
{
    links.push_back(dep);
}

void PropertyDependencyLinkList::removeDependency(const PropertyDependencyLink &dep)
{
    std::vector<PropertyDependencyLink>::iterator i = std::find(links.begin(), links.end(), dep);

    if (i != links.end())
        links.erase(i);
}

void PropertyDependencyLinkList::removeDependencies(const Path &path, void *owner)
{
   std::vector<PropertyDependencyLink>::iterator i = links.begin();

   while (i != links.end()) {
       if (i->getOwner() == owner && i->getPath() == path)
           i = links.erase(i);
       else
           ++i;
   }
}

Path::Component::Component(const std::string &_component, Path::Component::typeEnum _type, int _index, std::string _key)
    : component(_component)
    , type(_type)
    , index(_index)
    , key(_key)
{
}

Path::Component Path::Component::SimpleComponent(const std::string &_component)
{
    return Component(_component);
}

Path::Component Path::Component::ArrayComponent(const std::string &_component, int _index)
{
    return Component(_component, ARRAY, _index);
}

Path::Component Path::Component::MapComponent(const std::string &_component, const std::string & _key)
{
    return Component(_component, MAP, -1, _key);
}

bool Path::Component::operator ==(const Path::Component &other) const
{
    if (type != other.type)
        return false;

    if (component != other.component)
        return false;

    switch (type) {
    case SIMPLE:
        return true;
    case ARRAY:
        return index == other.index;
    case MAP:
        return key == other.key;
    default:
        assert(0);
    }
}
