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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "PopupDropperFactory.h"

#include "Debug.h"
#include "ContextView.h"
#include "PaletteHandler.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "context/popupdropper/libpud/PopupDropperItem.h"
#include "MainWindow.h"
#include "SvgHandler.h"

#include <kglobal.h>


namespace The
{
    static PopupDropperFactory* s_PopupDropperFactory_instance = 0;

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


PopupDropper * PopupDropperFactory::createPopupDropper( QWidget * parent )
{
    DEBUG_BLOCK

    PopupDropper* pd = new PopupDropper( parent );
    if( !pd )
        return 0;

    pd->setSvgRenderer( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ) );
    pd->setQuitOnDragLeave( false );
    pd->setFadeInTime( 500 );
    pd->setFadeOutTime( 300 );
    //QColor origWindowColor( The::paletteHandler()->palette().color( QPalette::Window ) );
    //QColor windowColor;
    //windowColor.setRed( 255 - origWindowColor.red() );
    //windowColor.setBlue( 255 - origWindowColor.blue() );
    //windowColor.setGreen( 255 - origWindowColor.green() );
    QColor windowColor( The::paletteHandler()->palette().color( QPalette::Base ) );
    windowColor.setAlpha( 176 );
    QColor textColor( The::paletteHandler()->palette().color( QPalette::Link ) );
    QColor highlightedTextColor( The::paletteHandler()->palette().color( QPalette::LinkVisited ) );
    QColor borderColor( The::paletteHandler()->palette().color( QPalette::Text ) );
    QColor fillColor( borderColor );
    fillColor.setAlpha( 48 );
    pd->setColors( windowColor, textColor, highlightedTextColor, borderColor, fillColor );

    return pd;
}

PopupDropper * PopupDropperFactory::createPopupDropper()
{
    return createPopupDropper( Context::ContextView::self() );
}

PopupDropperItem * PopupDropperFactory::createItem( PopupDropperAction * action )
{
    QFont font;
    font.setPointSize( 16 );
    font.setBold( true );

    PopupDropperItem* pdi = new PopupDropperItem();
    pdi->setAction( action );
    QString text = pdi->text();
    text.remove( QChar('&') );
    pdi->setText( text );
    pdi->setFont( font );
    pdi->setHoverMsecs( 800 );
    QColor hoverIndicatorFillColor( The::paletteHandler()->palette().color( QPalette::Highlight ) );
    hoverIndicatorFillColor.setAlpha( 96 );
    QBrush brush = pdi->hoverIndicatorFillBrush();
    brush.setColor( hoverIndicatorFillColor );
    pdi->setHoverIndicatorFillBrush( brush );

    return pdi;
}

void PopupDropperFactory::adjustSubmenuItem( PopupDropperItem *item )
{
    if( !item )
        return;

    QFont font;
    font.setPointSize( 16 );
    font.setBold( true );

    item->setFont( font );
    item->setHoverMsecs( 800 );
    item->setHoverIndicatorShowStyle( PopupDropperItem::OnHover );
    QColor hoverIndicatorFillColor( The::paletteHandler()->palette().color( QPalette::Highlight ) );
    hoverIndicatorFillColor.setAlpha( 96 );
    QBrush brush = item->hoverIndicatorFillBrush();
    brush.setColor( hoverIndicatorFillColor );
    item->setHoverIndicatorFillBrush( brush );
}

