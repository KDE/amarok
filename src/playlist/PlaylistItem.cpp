/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "ItemScene.h"
#include "PlaylistItem.h"

PlaylistNS::Item::Item( Meta::TrackPtr track )
    : m_track( track )
    , m_scene( new PlaylistNS::ItemScene( track ) )
{ }

PlaylistNS::Item::~Item()
{
    DEBUG_BLOCK
    delete m_scene;
}

QGraphicsScene* 
PlaylistNS::Item::scene( int totalWidth )
{
    m_scene->resize( totalWidth );
    return m_scene;
}

