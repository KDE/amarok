/***************************************************************************
                       void-engine.h - Dummy engine plugin

copyright            : (C) 2003 by Max Howell <max.howell@methylblue.com>
copyright            : (C) 2004 by Mark Kretschmann <markey@web.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "void-engine.h"

#include <klocale.h>


AMAROK_EXPORT_PLUGIN( VoidEngine )


bool
VoidEngine::load( const KURL& url, bool stream )
{
    Engine::Base::load( url, stream );
    emit statusText( i18n( "Error: No engine loaded, cannot start playback." ) );

    return false;
}

