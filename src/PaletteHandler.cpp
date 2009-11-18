/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "PaletteHandler.h"

#include "App.h"
#include "Debug.h"
#include "MainWindow.h"

#include <kglobal.h>


namespace The {
    static PaletteHandler* s_PaletteHandler_instance = 0;

    PaletteHandler* paletteHandler()
    {
        if( !s_PaletteHandler_instance )
            s_PaletteHandler_instance = new PaletteHandler();

        return s_PaletteHandler_instance;
    }
}


PaletteHandler::PaletteHandler( QObject* parent )
    : QObject( parent )
{}


PaletteHandler::~PaletteHandler()
{
    DEBUG_BLOCK

    The::s_PaletteHandler_instance = 0;
}

void
PaletteHandler::setPalette( const QPalette & palette )
{
    m_palette = palette;
    emit( newPalette( m_palette ) );
}

void
PaletteHandler::updateItemView( QAbstractItemView * view )
{
    QPalette p = m_palette;

    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );

    c = p.color( QPalette::AlternateBase );
    c.setAlpha( 77 );
    p.setColor( QPalette::AlternateBase, c );

    view->setPalette( p );
}

QPalette
PaletteHandler::palette()
{
    return m_palette;
}

QColor
PaletteHandler::highlightColor()
{
    QColor highlight = App::instance()->palette().highlight().color();
    qreal saturation = highlight.saturationF();
    saturation *= 0.5;
    highlight.setHsvF( highlight.hueF(), saturation, highlight.valueF(), highlight.alphaF() );

    return highlight;
}

QColor
PaletteHandler::highlightColor( qreal saturationPercent, qreal valuePercent )
{
    QColor highlight = QColor( App::instance()->palette().highlight().color() );
    qreal saturation = highlight.saturationF();
    saturation *= saturationPercent;
    qreal value = highlight.valueF();
    value *= valuePercent;
    highlight.setHsvF( highlight.hueF(), saturation, value, highlight.alphaF() );

    return highlight;
}

QColor
PaletteHandler::backgroundColor()
{
    return App::instance()->palette().base().color();
}

QColor
PaletteHandler::alternateBackgroundColor()
{
    const QColor alternate = App::instance()->palette().alternateBase().color();
    const QColor window    = App::instance()->palette().window().color();
    const QColor base      = backgroundColor();

    const int alternateDist = abs( alternate.value() - base.value() );
    const int windowDist    = abs( window.value()    - base.value() );

    if( alternateDist > windowDist )
        return alternate;
    else
        return window;
}

#include "PaletteHandler.moc"
