/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *             (C) 2004 Frederik Holljen <fh@ez.no>                        *
 *             (C) 2005 Gábor Lehel <illissius@gmail.com>                  *
 *             (C) 2007 Seb Ruiz <ruiz@kde.org>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.             *
 ***************************************************************************/

#include "amarokconfig.h"
#include "amarok.h"
#include "debug.h"
#include "meta/Meta.h"
#include "meta/MetaUtility.h"
#include "playlist/PlaylistModel.h"
#include "PlaylistStatusBar.h"
#include "TheInstances.h"

#include <khbox.h>
#include <kiconloader.h>
#include <klocale.h>

#include <QLabel>
#include <QLayout>
#include <QTextDocument>
#include <QTimer>

// stuff that must be included last
//#include "startupTips.h"
#include "timeLabel.h"


namespace Amarok {


KAction *action( const char *name ) { return (KAction*)Amarok::actionCollection()->action( name ); }

//TODO disable hide statusbar? or show when required? that sucks though.


PlaylistStatusBar* PlaylistStatusBar::s_instance = 0;


PlaylistStatusBar::PlaylistStatusBar( QWidget *parent, const char *name )
        : KDE::StatusBar( parent, name )
{
    s_instance = this; //static member
    setSizeGripEnabled( false );
    setMainText( "Test 1 two three four" );
    // total songs count
    QWidget *lengthBox = new QWidget( this );
    addPermanentWidget( lengthBox );
    QHBoxLayout *lengthLayout = new QHBoxLayout( lengthBox );
    lengthLayout->setMargin(1);
    lengthLayout->setSpacing(2);
    lengthLayout->addSpacing(3);
    m_itemCountLabel = new QLabel( lengthBox );
    lengthLayout->addWidget( m_itemCountLabel );
    lengthLayout->addSpacing( 3 );
    lengthBox->setLayout( lengthLayout );

    m_itemCountLabel->setAlignment( Qt::AlignCenter );
    m_itemCountLabel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    QWidget *hbox = new QWidget( this );
    addPermanentWidget(hbox);
    QHBoxLayout *layout = new QHBoxLayout( hbox );
    layout->setMargin(0);
    layout->setSpacing(2);

    layout->addSpacing( 3 );
    //PORT 2.0
//     layout->addWidget( m_queueLabel = new QueueLabel( hbox ) );
    layout->addSpacing( 3 );

    //TODO reimplement insertChild() instead
    addPermanentWidget( m_itemCountLabel );
    addPermanentWidget( hbox );

    // for great justice!
    connect( The::playlistModel(), SIGNAL(playlistCountChanged(int)),
                                    SLOT(slotItemCountChanged(int)) );

    //session stuff
    //setShown( AmarokConfig::showStatusBar() );
}

void
PlaylistStatusBar::slotItemCountChanged( int newCount )
{
//     const bool hasSel = ( selCount > 1 ), hasVis = ( visCount != newCount );
// 
//     QString text = ( hasSel && hasVis ) ? i18n( "%1 selected of %2 visible tracks",selCount, visCount )
//                  : ( hasVis && newCount == 1 ) ? i18n( "0 visible of 1 track" )
//                  : ( hasVis ) ? i18n( "%1 visible of %2 tracks", visCount, newCount )
//                  : ( hasSel ) ? i18n( "%1 selected of %2 tracks", selCount, newCount )
//                  : i18np( "1 track", "%1 tracks", newCount );
// 
//     int getValue = 0;
// 
//     if( hasSel )
//         getValue = selLength;
// 
//     else if( hasVis )
//         getValue = visLength;
// 
//     else
//         getValue = newLength;
// 
//     if( getValue )
//         m_itemCountLabel->setText( i18nc( "X visible/selected tracks (time) ", "%1 (%2)", text, MetaBundle::fuzzyTime( getValue ) ) );
//     else
//         m_itemCountLabel->setText( text );
// 
//     m_itemCountLabel->setToolTip(  i18n( "Play-time: %1", MetaBundle::veryPrettyTime( getValue ) ) );
    uint totalSeconds = 0;
    QList<Playlist::Item*> items;
    items = The::playlistModel()->itemList();
    foreach( Playlist::Item* item, items )
    {
        totalSeconds += item->track()->length();
    }
    m_itemCountLabel->setText( i18n( "%1 Tracks (%2)", newCount, Meta::secToPrettyTime(totalSeconds) ) );
    m_itemCountLabel->setToolTip( i18n( "Play-time: %1", Meta::secToPrettyTime( totalSeconds ) ) );
}

#include "PlaylistStatusBar.moc"
