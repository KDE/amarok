/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_APPLET_H
#define AMAROK_APPLET_H

#include "amarok_export.h"
#include <plasma/applet.h>

namespace Context
{

typedef Plasma::Applet Applet;
// here we re-implement plasma-specific functions to make them amarok-specific

/*
class AMAROK_EXPORT Applet : public Plasma::Applet
{
    Q_OBJECT
public:
    Applet( QObject* parent, const QStringList& args);
    
    // TODO : this function stores its data in the d pointer, 
    // so we can't override it with out own. PROBLEM! as applets
    // that use this would save to plasma_appletname config files.
    //KConfigGroup globalAppletConfig() const;
    
    static KPluginInfo::List knownApplets();
    static Applet* loadApplet(const QString& appletName, uint appletId, const QStringList& args);
    
};
*/
} // Context namespace
/**
 * Register an applet when it is contained in a loadable module
 */
#define K_EXPORT_AMAROK_APPLET(libname, classname) \
K_EXPORT_COMPONENT_FACTORY(                \
    amarok_context_applet_##libname,    \
    KGenericFactory<classname>("amarok_context_applet_" #libname))
    
#endif // multiple inclusion guard
