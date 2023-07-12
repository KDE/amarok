/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#define DEBUG_PREFIX "PaletteHandler"
 
#include "PaletteHandler.h"

#include <QAbstractItemView>
#include <QPainter>


namespace The {
    static PaletteHandler* s_PaletteHandler_instance = nullptr;

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
    The::s_PaletteHandler_instance = nullptr;
}

void
PaletteHandler::setPalette( const QPalette & palette )
{
    m_palette = palette;
    Q_EMIT( newPalette( m_palette ) );
}

void
PaletteHandler::updateItemView( QAbstractItemView * view )
{
    QPalette p = m_palette;
    QColor c;

    // Widgets with keyboard focus become slightly transparent
    c = p.color( QPalette::Active, QPalette::AlternateBase );
    c.setAlpha( 95 );
    p.setColor( QPalette::Active, QPalette::AlternateBase, c );

    // For widgets that don't have keyboard focus reduce the opacity further
    c = p.color( QPalette::Inactive, QPalette::AlternateBase );
    c.setAlpha( 75 );
    p.setColor( QPalette::Inactive, QPalette::AlternateBase, c );

    // Base color is used during the expand/shrink animation. We set it
    // to transparent so that it won't interfere with our custom colors.
    p.setColor( QPalette::Active, QPalette::Base, Qt::transparent );
    p.setColor( QPalette::Inactive, QPalette::Base, Qt::transparent );

    view->setPalette( p );
    
    if ( QWidget *vp = view->viewport() )
    {
        // don't paint background - do NOT use Qt::transparent etc.
        vp->setAutoFillBackground( false );
        vp->setBackgroundRole( QPalette::Window );
        vp->setForegroundRole( QPalette::WindowText );
        // erase custom viewport palettes, shall be "transparent"
        vp->setPalette(QPalette());
    }
}

QColor
PaletteHandler::foregroundColor( const QPainter *p, bool selected )
{
    QPalette pal;
    QPalette::ColorRole fg = QPalette::WindowText;
    if ( p->device() && p->device()->devType() == QInternal::Widget)
    {
        QWidget *w = static_cast<QWidget*>( p->device() );
        fg = w->foregroundRole();
        pal = w->palette();
    }
    else
        pal = palette();

    if( !selected )
        return pal.color( QPalette::Active, fg );

    return pal.color( QPalette::Active, QPalette::HighlightedText );
}

QPalette
PaletteHandler::palette() const
{
    return m_palette;
}

QColor
PaletteHandler::highlightColor( qreal saturationPercent, qreal valuePercent )
{
    QColor highlight = The::paletteHandler()->palette().color( QPalette::Active, QPalette::Highlight );
    qreal saturation = highlight.saturationF();
    saturation *= saturationPercent;
    qreal value = highlight.valueF();
    value *= valuePercent;
    if( value > 1.0 )
        value = 1.0;
    highlight.setHsvF( highlight.hueF(), saturation, value, highlight.alphaF() );

    return highlight;
}

QColor
PaletteHandler::backgroundColor()
{
    QColor base = The::paletteHandler()->palette().color( QPalette::Active, QPalette::Base );
    base.setHsvF( highlightColor().hueF(), base.saturationF(), base.valueF() );
    return base;
}

QColor
PaletteHandler::alternateBackgroundColor()
{
    const QColor alternate = The::paletteHandler()->palette().color( QPalette::Active, QPalette::AlternateBase );
    const QColor window    = The::paletteHandler()->palette().color( QPalette::Active, QPalette::Window );
    const QColor base      = backgroundColor();

    const int alternateDist = abs( alternate.value() - base.value() );
    const int windowDist    = abs( window.value()    - base.value() );

    QColor result = alternateDist > windowDist ? alternate : window;
    result.setHsvF( highlightColor().hueF(), highlightColor().saturationF(), result.valueF() );
    return result;
}

