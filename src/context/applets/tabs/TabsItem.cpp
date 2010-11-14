/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#include "TabsItem.h"
#include "SvgHandler.h"

#include <KStandardDirs>

#include <QIcon>
#include <QPixmap>

TabsItem::TabsItem()
    : QStandardItem()
    , m_iconSize( 0 )
    , m_tabsInfo( 0 )
{
    m_tabsInfo = new TabsInfo();
    setEditable( false );
    setIconSize( 36 );
}

void
TabsItem::setTab( TabsInfo *tab )
{
    if( tab )
    {
        m_tabsInfo = tab;
        setTabIcon( tab->tabType );
        setToolTip( m_tabsInfo->title );
    }
}

void
TabsItem::setIconSize( const int iconSize )
{
    static const int padding = 5;

    m_iconSize = iconSize;

    QSize size = sizeHint();
    size.setHeight( iconSize + padding * 2 );
    setSizeHint( size );
}

void
TabsItem::setTabIcon( TabsInfo::TabType tabtype )
{
    QPixmap pix;
    switch( tabtype )
    {
        case TabsInfo::GUITAR:
            pix = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-tabs-guitar.png" ) );
            break;
        case TabsInfo::BASS:
            pix = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-tabs-bass.png" ) );
            break;
        case TabsInfo::DRUM:
            pix = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-tabs-drum.png" ) );
            break;
        case TabsInfo::PIANO:
            pix = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-tabs-piano.png" ) );
            break;
    }

    QPixmap pixwithBorder( The::svgHandler()->addBordersToPixmap( pix, 3, "Thumbnail", true ) ) ;
    setIcon( QIcon( pixwithBorder ) );
}

const QString
TabsItem::getTabData()
{
    if( m_tabsInfo )
        return m_tabsInfo->tabs;
    else
        return QString();
}

const QString
TabsItem::getTabTitle()
{
    if( m_tabsInfo )
        return m_tabsInfo->title;
    else
        return QString();
}

const QString
TabsItem::getTabUrl()
{
    if( m_tabsInfo )
        return m_tabsInfo->url.url();
    else
        return QString();
}


const QString
TabsItem::getTabSource()
{
    if( m_tabsInfo )
        return m_tabsInfo->source;
    else
        return QString();
}
