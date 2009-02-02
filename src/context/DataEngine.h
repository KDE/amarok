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

#ifndef AMAROK_DATA_ENGINE_H
#define AMAROK_DATA_ENGINE_H

#include <plasma/dataengine.h>

namespace Context
{
    typedef Plasma::DataEngine DataEngine;

} // context  namespace

#define K_EXPORT_AMAROK_DATAENGINE(libname, classname) \
K_PLUGIN_FACTORY(factory, registerPlugin<classname>();) \
K_EXPORT_PLUGIN(factory("amarok_data_engine_" #libname))\
K_EXPORT_PLUGIN_VERSION(PLASMA_VERSION)
#endif

