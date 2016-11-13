#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "BodyExtension.h"

using namespace App;

EXTENSION_PROPERTY_SOURCE(App::BodyExtension, App::Extension)

BodyExtension::BodyExtension()
{
    initExtension(BodyExtension::getExtensionClassTypeId());

    EXTENSION_ADD_PROPERTY(Tip,(nullptr));
}

BodyExtension::~BodyExtension()
{

}

// Python feature ---------------------------------------------------------

namespace App {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::BodyExtensionPython, App::BodyExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<ExtensionPythonT<BodyExtension>>;
}
