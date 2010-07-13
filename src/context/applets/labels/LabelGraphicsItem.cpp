/****************************************************************************************
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#include "LabelGraphicsItem.h"

#include "PaletteHandler.h"

// KDE
#include <KIconLoader>

// Qt
#include <QFont>
#include <QGraphicsSceneHoverEvent>


LabelGraphicsItem::LabelGraphicsItem( const QString &text, int deltaPointSize, QGraphicsItem *parent )
    : QGraphicsTextItem( text, parent ),
    m_selected( false )
{
    setAcceptHoverEvents( true );

    KIconLoader *iconLoader = new KIconLoader();
    m_addLabelPixmap = iconLoader->loadIcon( "list-add", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium );
    m_removeLabelPixmap = iconLoader->loadIcon( "list-remove", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium );
    m_blacklistLabelPixmap = iconLoader->loadIcon( "flag-black", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium );
    m_listLabelPixmap = iconLoader->loadIcon( "view-list-text", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium );
    delete iconLoader;

    m_addLabelItem = new QGraphicsPixmapItem( this );
    m_addLabelItem->setToolTip( i18n( "Add label" ) );
    m_addLabelItem->hide();
    m_removeLabelItem = new QGraphicsPixmapItem( this );
    m_removeLabelItem->setToolTip( i18n( "Remove label" ) );
    m_removeLabelItem->hide();
    m_blacklistLabelItem = new QGraphicsPixmapItem( this );
    m_blacklistLabelItem->setToolTip( i18n( "Add to blacklist" ) );
    m_blacklistLabelItem->hide();
    m_listLabelItem = new QGraphicsPixmapItem( this );
    m_listLabelItem->setToolTip( i18n( "Show top tracks" ) );
    m_listLabelItem->hide();

    setDeltaPointSize( deltaPointSize );
}

LabelGraphicsItem::~LabelGraphicsItem()
{}

void LabelGraphicsItem::setDeltaPointSize( int deltaPointSize )
{
    QFont f = font();
    f.setPointSize( f.pointSize() + deltaPointSize );
    setFont( f );

    int iconsCount = 2 + 1;
    int maxHeight = boundingRect().height() * 2 / 3;
    int maxWidth = 0;
    while( maxWidth < 14 )
    {
        iconsCount--;
        maxWidth = boundingRect().width() / iconsCount;
    }
    int iconsSize = maxHeight < maxWidth ? maxHeight : maxWidth;
    const int iconsSpaceA = ( boundingRect().width() - iconsSize * iconsCount ) / ( iconsCount - 1 );
    const int iconsSpaceB = iconsSize / 2;
    int iconsSpace = iconsSpaceA < iconsSpaceB ? iconsSpaceA : iconsSpaceB;

    m_addLabelItem->setPixmap( m_addLabelPixmap.scaledToHeight( iconsSize, Qt::SmoothTransformation ) );
    m_addLabelItem->setPos( 0, ( boundingRect().height() - iconsSize ) / 2 );
    m_removeLabelItem->setPixmap( m_removeLabelPixmap.scaledToHeight( iconsSize, Qt::SmoothTransformation ) );
    m_removeLabelItem->setPos( 0, ( boundingRect().height() - iconsSize ) / 2 );
    m_blacklistLabelItem->setPixmap( m_blacklistLabelPixmap.scaledToHeight( iconsSize, Qt::SmoothTransformation ) );
    m_blacklistLabelItem->setPos( iconsSize + iconsSpace, ( boundingRect().height() - iconsSize ) / 2 );
    m_blacklistLabelItem->setEnabled( iconsCount >= 2 );
    m_listLabelItem->setPixmap( m_listLabelPixmap.scaledToHeight( iconsSize, Qt::SmoothTransformation ) );
    m_listLabelItem->setPos( iconsSize * 2 + iconsSpace * 2, ( boundingRect().height() - iconsSize ) / 2 );
    m_listLabelItem->setEnabled( iconsCount >= 3 );
}

void LabelGraphicsItem::setSelected( bool selected )
{
    m_selected = selected;
    if( m_selected )
        setDefaultTextColor( QColor(0, 110, 0) );
    else
        setDefaultTextColor( QColor(0, 0, 0) );
    
    if( isUnderMouse() )
    {
        m_addLabelItem->hide();
        m_removeLabelItem->hide();
        m_blacklistLabelItem->hide();
        m_listLabelItem->hide();
        hoverEnterEvent( 0 );
    }
    else
    {
        update();
    }
}

void LabelGraphicsItem::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )

    setDefaultTextColor( PaletteHandler::highlightColor( 0.7, 1.0 ) );

    if( m_selected )
        m_removeLabelItem->show();
    else
        m_addLabelItem->show();
    
    if( m_blacklistLabelItem->isEnabled() )
        m_blacklistLabelItem->show();
    
    if( m_listLabelItem->isEnabled() )
        m_listLabelItem->show();
    
    update();
}

void LabelGraphicsItem::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    
    m_addLabelItem->hide();
    m_removeLabelItem->hide();
    m_blacklistLabelItem->hide();
    m_listLabelItem->hide();
    
    if( m_selected )
        setDefaultTextColor( QColor(0, 110, 0) );
    else
        setDefaultTextColor( QColor(0, 0, 0) );
    
    update();
}

void LabelGraphicsItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( m_addLabelItem->contains( mapToItem( m_addLabelItem, event->pos() ) ) || m_removeLabelItem->contains( mapToItem( m_removeLabelItem, event->pos() ) ) )
        emit toggled( toPlainText() );
    else if( m_blacklistLabelItem->isEnabled() && m_blacklistLabelItem->contains( mapToItem( m_blacklistLabelItem, event->pos() ) ) )
        emit blacklisted( toPlainText() );
    else if( m_listLabelItem->isEnabled() && m_listLabelItem->contains( mapToItem( m_listLabelItem, event->pos() ) ) )
        emit list( toPlainText() );
}

