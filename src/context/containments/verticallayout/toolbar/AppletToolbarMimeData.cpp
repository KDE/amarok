/**************************************************************************
* copyright            : (C) 2008 Leo Franchi <lfranchi@kde.org  >        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "AppletToolbarMimeData.h"

#include "AppletToolbarAppletItem.h"

Context::AppletToolbarMimeData::AppletToolbarMimeData()
    : QMimeData()
    , m_applet( 0 )
    , m_location( 0 )
{
    
}

Context::AppletToolbarMimeData::~AppletToolbarMimeData()
{
    
}

void 
Context::AppletToolbarMimeData::setAppletData( Context::AppletToolbarAppletItem* applet )
{
    m_applet = applet;
}

Context::AppletToolbarAppletItem* 
Context::AppletToolbarMimeData::appletData() const
{
    return m_applet;
}

void 
Context::AppletToolbarMimeData::setLocationData( int loc )
{
    m_location = loc;
}

int 
Context::AppletToolbarMimeData::locationData() const
{
    return m_location;
}

