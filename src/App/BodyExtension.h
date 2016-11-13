#ifndef APP_Body_H
#define APP_Body_H

#include <App/FeaturePython.h>

#include "GroupExtension.h"
#include "PropertyGeo.h"
#include "Extension.h"

namespace App
{

/**
 * class BodyExtension: base class for body-like behaving objects. Provides Tip
 * property, plus two display modes (Tip and Through; see corresponding
 * ViewProvider), that set how the body is presented in 3d view. Tip property
 * is supposed to be pointing to the final object, and the rest of the objects
 * contained by body are "private". It is supposed that the type of object
 * being extended is similar to the type of one pointed by Tip (e.g., object
 * being extended should be Part::Feature, if Tip points to Part::Feature, and
 * copy Tip's shape to be its own shape.).
 *
 * BodyExtension does not provide sequence management, or tip management.
 *
 * BodyExtension requires that GroupExtension-derived extension is added to the
 * object being extended.
 */
class AppExport BodyExtension : public App::Extension
{
    EXTENSION_PROPERTY_HEADER(App::BodyExtension);

public:
    PropertyLink Tip;

    /// Constructor
    BodyExtension(void);
    virtual ~BodyExtension();
};

typedef ExtensionPythonT<ExtensionPythonT<BodyExtension>> BodyExtensionPython;


} //namespace App


#endif // APP_Body_H
