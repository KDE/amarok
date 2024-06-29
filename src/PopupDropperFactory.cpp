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

#include "PopupDropperFactory.h"

#include "MainWindow.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "core/support/Debug.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"

#include <QAction>


namespace The
{
    static PopupDropperFactory* s_PopupDropperFactory_instance = nullptr;

    PopupDropperFactory* popupDropperFactory()
    {
        if( !s_PopupDropperFactory_instance )
            s_PopupDropperFactory_instance = new PopupDropperFactory( The::mainWindow() );

        return s_PopupDropperFactory_instance;
    }
}


PopupDropperFactory::PopupDropperFactory( QObject* parent )
    : QObject( parent )
{}


PopupDropperFactory::~PopupDropperFactory()
{
    DEBUG_BLOCK
}


PopupDropper * PopupDropperFactory::createPopupDropper( QWidget * parent, bool ignoreEmptyParent )
{
    DEBUG_BLOCK

    // Lazy loading of widgets not currently shown in layout means that parent could be zero
    // if this happens, it pops up in its own window -- so detect this
    // ignoreEmptyParent is for creating submenus, where you set the initial parent to zero
    if( !parent && !ignoreEmptyParent )
        return nullptr;

    PopupDropper* pd = new PopupDropper( parent );
    if( !pd )
        return nullptr;

    pd->setSvgRenderer( The::svgHandler()->getRenderer( QStringLiteral("amarok/images/pud_items.svg") ) );
    pd->setQuitOnDragLeave( false );
    pd->setFadeInTime( 500 );
    pd->setFadeOutTime( 300 );
    //QColor origWindowColor( The::paletteHandler()->palette().color( QPalette::Window ) );
    //QColor windowColor;
    //windowColor.setRed( 255 - origWindowColor.red() );
    //windowColor.setBlue( 255 - origWindowColor.blue() );
    //windowColor.setGreen( 255 - origWindowColor.green() );
    QColor windowColor( The::paletteHandler()->palette().color( QPalette::Base ) );
    windowColor.setAlpha( 200 );
    QColor textColor( The::paletteHandler()->palette().color( QPalette::Link ) );
    QColor highlightedTextColor( The::paletteHandler()->palette().color( QPalette::Text ) );
    QColor borderColor( The::paletteHandler()->palette().color( QPalette::Text ) );
    QColor fillColor( borderColor );
    fillColor.setAlpha( 48 );
    pd->setColors( windowColor, textColor, highlightedTextColor, borderColor, fillColor );

    return pd;
}

PopupDropperItem * PopupDropperFactory::createItem( QAction * action )
{
    PopupDropperItem* pdi = new PopupDropperItem();
    pdi->setAction( action );
    QString text = pdi->text();
    text.remove( QLatin1Char('&') );
    pdi->setText( text );
    adjustItem( pdi );
    return pdi;
}

void PopupDropperFactory::adjustItem( PopupDropperItem *item )
{
    if( !item )
        return;
    QFont font;
    font.setPointSize( 16 );
    font.setBold( true );
    item->setFont( font );
    item->setHoverMsecs( 800 );
    QColor hoverIndicatorFillColor( The::paletteHandler()->palette().color( QPalette::Highlight ) );
    hoverIndicatorFillColor.setAlpha( 96 );
    QBrush brush = item->hoverIndicatorFillBrush();
    brush.setColor( hoverIndicatorFillColor );
    item->setHoverIndicatorFillBrush( brush );

    if( item->isSubmenuTrigger() )
        item->setHoverIndicatorShowStyle( PopupDropperItem::OnHover );
}

void PopupDropperFactory::adjustItems( PopupDropper* pud )
{
    if( !pud )
        return;
    pud->forEachItem( adjustItemCallback );
}

void PopupDropperFactory::adjustItemCallback( void *pdi )
{
    The::popupDropperFactory()->adjustItem( (PopupDropperItem*)pdi );
}

