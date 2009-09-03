/****************************************************************************************
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistHeader.h"

#include "Debug.h"

#include <KLocale>

#include <QDrag>
#include <QDropEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QVBoxLayout>

const QString Playlist::HeaderWidget::HeaderMimeType = "application/x-amarok-playlist-header";

Playlist::HeaderWidget::HeaderWidget( QWidget* parent )
        : QWidget( parent )
        , m_topLayout( new QHBoxLayout( this ) )
{
    DEBUG_BLOCK
    m_test << i18n( "Artist" ) << i18n( "Track Number - Title" ) << i18n( "Album" ) << i18n( "Length" );
    m_topLayout->setSpacing( 0 );
    QFont smallFont;
    smallFont.setPointSize( 8 );
    setFont( smallFont );
    for ( int i = 0; i < 2; i++ )
    {
        m_verticalLayouts.push_back( new QVBoxLayout( this ) );
        m_verticalLayouts.at( i )->setSpacing( 0 );
        m_topLayout->addLayout( m_verticalLayouts.at( i ) );
    }
    for ( int i = 0; i < 4; i++ )
    {
        m_labels.push_back( new QLabel( this ) );
        m_labels.at( i )->setFrameStyle( QFrame::StyledPanel );
        int column = ( i < 2 ) ? 0 : 1;
        m_verticalLayouts[column]->addWidget( m_labels.at( i ) );
        m_labels.at( i )->setText( m_test.at( i ) );
        m_textToLabel[ m_test.at( i )] = m_labels.at( i );
    }
    setAcceptDrops( true );
}

void
Playlist::HeaderWidget::dragEnterEvent( QDragEnterEvent *event )
{
    if ( event->mimeData()->hasFormat( HeaderMimeType )  && event->source() == this )
    {
        event->accept();
    }
    else
        event->ignore();
}

void
Playlist::HeaderWidget::dropEvent( QDropEvent *event )
{
    if ( event->mimeData()->hasFormat( HeaderMimeType ) )
    {
        QByteArray itemData = event->mimeData()->data( HeaderMimeType );
        QDataStream dataStream( &itemData, QIODevice::ReadOnly );
        QString name;
        dataStream >> name;
        QLabel* sourceLabel = m_textToLabel[ name ];
        QLabel* droppedOnLabel = dynamic_cast<QLabel*>( childAt( event->pos() ) );
        if ( !droppedOnLabel )
            return;
        QString origSource = sourceLabel->text();
        sourceLabel->setText( droppedOnLabel->text() );
        droppedOnLabel->setText( origSource );
        m_textToLabel[ sourceLabel->text()] = sourceLabel;
        m_textToLabel[ droppedOnLabel->text()] = droppedOnLabel;
        event->accept();
    }
    else
        event->ignore();
}

void
Playlist::HeaderWidget::enterEvent( QEvent* event )
{
    Q_UNUSED( event );
}

void
Playlist::HeaderWidget::leaveEvent( QEvent* event )
{
    Q_UNUSED( event );
}

void
Playlist::HeaderWidget::mousePressEvent( QMouseEvent *event )
{
    QLabel *child = dynamic_cast<QLabel*>( childAt( event->pos() ) );
    if ( !child ) return;

    QByteArray itemData;
    QDataStream dataStream( &itemData, QIODevice::WriteOnly );
    dataStream << child->text();
    QMimeData* mimeData = new QMimeData;
    mimeData->setData( HeaderMimeType, itemData );

    QDrag* drag = new QDrag( this );
    QPixmap labelPixmap = QPixmap::grabWidget( child );
    drag->setPixmap( labelPixmap );
    drag->setMimeData( mimeData );
    //drag->setHotSpot( QPoint( labelPixmap.width()/2, labelPixmap.height() ) );
    drag->exec();
}

