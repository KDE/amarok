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

#include "PopupDropper.h"

#include "App.h"
#include "debug.h"
#include "contextview/contextview.h"
#include "PopupDropperScene.h"
#include "PopupDropperView.h"

#include <QBrush>
#include <QColor>

static bool ENABLED = true;

using namespace PopupDropperNS;

PopupDropper* PopupDropper::s_instance = 0;

PopupDropper*
PopupDropper::instance()
{
    static PopupDropper pd;
    return &pd;
}

PopupDropper::PopupDropper() : QObject()
    , m_scene( 0 )
    , m_view( 0 )
    , m_enabled( ENABLED )
    , m_initialized( false )
{
    DEBUG_BLOCK
    s_instance = this;
}

PopupDropper::~PopupDropper()
{
    DEBUG_BLOCK
    //m_scene.setPDV( 0 );
    //delete m_view;
    //m_view = 0;
}

void
PopupDropper::initialize( QWidget* window )
{
    DEBUG_BLOCK
    if( !window )
        return;
    m_scene.setParent( window );
    m_scene.setSceneRect( QRectF( window->rect() ) );
    m_view = new PopupDropperView( &m_scene, window );
    m_scene.setPDV( m_view );
    m_initialized = true;
}

void
PopupDropper::create()
{
    DEBUG_BLOCK
    if( !m_initialized )
        initialize( ContextView::instance() );
    if( !m_scene.isShown() )
    {
        const QWidget *parent = static_cast<QWidget *>( m_scene.parent() );
        m_scene.setSceneRect( QRectF( parent->rect() ) );
        m_view->resize( parent->size() + QSize( 2, 2 ) );
        m_scene.startPDV();
    }
}

void
PopupDropper::destroy()
{
    DEBUG_BLOCK
    if( m_scene.isShown() )
        m_scene.stopPDV();
}

namespace The {
    PopupDropperNS::PopupDropper* PopupDropper() { return PopupDropperNS::PopupDropper::instance(); }
}

#include "PopupDropper.moc"

