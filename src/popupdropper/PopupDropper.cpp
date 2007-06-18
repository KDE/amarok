/*
 *  Copyright (c) 2007 Jeff Mitchell <kde-dev@emailgoeshere.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "app.h"
#include "debug.h"
#include "PopupDropper.h"
#include "PopupDropperScene.h"
#include "PopupDropperView.h"

#include <QBrush>
#include <QColor>

using namespace PopupDropperNS;

PopupDropper* PopupDropper::s_instance = 0;

PopupDropper*
PopupDropper::instance()
{
    static PopupDropper pd;
    return &pd;
}

PopupDropper::PopupDropper() : QObject(),
    m_scene(0)
{
    DEBUG_BLOCK
    m_view = 0;
    s_instance = this;
}

PopupDropper::~PopupDropper()
{
    delete m_view;
}

void
PopupDropper::Initialize( QWidget* window )
{
    DEBUG_BLOCK
    if( !window )
        return;
    m_scene.setSceneRect( QRectF( window->rect() ) );
    m_view = new PopupDropperView( &m_scene, window );
    m_scene.setPDV( m_view );
    m_scene.startPDV();
    m_initialized = true;
}

void
PopupDropper::Destroy()
{
    DEBUG_BLOCK
    m_scene.stopPDV();
    m_view = 0;
    m_initialized = false;
}

namespace The {
    PopupDropperNS::PopupDropper* PopupDropper() { return PopupDropperNS::PopupDropper::instance(); }
}

#include "PopupDropper.moc"

