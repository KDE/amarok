/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#include "PhotosScrollWidget.h"
#include "DragPixmapItem.h"

// Amarok
#include "Debug.h"
#include "SvgHandler.h"

// KDE
#include <Plasma/Animator>

// QT
#include <QGraphicsPixmapItem>
#include <QGraphicsSceneHoverEvent>
#include <QList>
#include <QPixmap>

#define DEBUG_PREFIX "PhotosScrollWidget"

PhotosScrollWidget::PhotosScrollWidget( QGraphicsItem* parent )
    : QGraphicsWidget( parent )
    , m_id( 0 )
    , m_animating( false )
    , m_speed( 1. )
    , m_margin( 5 )
    , m_scrollmax( 0 )
    , m_actualpos( 0 )
    , m_mode( PHOTOS_MODE_INTERACTIVE )
{
    setAcceptHoverEvents( true );
    setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);
}

PhotosScrollWidget::~PhotosScrollWidget()
{
    DEBUG_BLOCK
    clear();
}


void PhotosScrollWidget::clear()
{
    if ( !m_id )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
    }
    
    foreach( QGraphicsItem *it, this->childItems() )
        delete it;

    m_currentlist.clear();
    m_scrollmax = 0;
    m_actualpos = 0;
}


void PhotosScrollWidget::setModeToInteractive()
{
    m_mode = PHOTOS_MODE_INTERACTIVE;
    QList < PhotosInfo * > tmp = m_currentlist;
    clear();
    setPixmapList( tmp );
    tmp.clear();
}
void PhotosScrollWidget::setModeToAutomatic()
{
    m_mode = PHOTOS_MODE_AUTOMATIC;
    QList < PhotosInfo * > tmp = m_currentlist;
    clear();
    setPixmapList( tmp );
    tmp.clear();
}
void PhotosScrollWidget::setModeToFading()
{
    m_mode = PHOTOS_MODE_FADING;
    QList < PhotosInfo * > tmp = m_currentlist;
    clear();
    setPixmapList( tmp );
    tmp.clear();
}

void PhotosScrollWidget::setPixmapList (QList < PhotosInfo * > list)
{
    
    // if the list is the same, nothing happen.
    if ( list == m_currentlist )
        return;

    // If a new one arrived, we change.
    foreach( PhotosInfo *item, list )
    {
        switch ( m_mode )
        {
            case PHOTOS_MODE_INTERACTIVE :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    if ( !m_id ) // carefull we're animating
                    {
                        Plasma::Animator::self()->stopCustomAnimation( m_id );
                        m_id = 0;
                    }
                    DragPixmapItem *dragpix = new DragPixmapItem( this );
                    dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                    item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                    dragpix->setPos( m_actualpos, 0 );
                    dragpix->SetClickableUrl( item->urlpage );
                    dragpix->show();

                    int delta = dragpix->boundingRect().width() + m_margin;
                    m_scrollmax += delta;
                    m_actualpos += delta;
                }
                break;
            }
            case PHOTOS_MODE_AUTOMATIC :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    if ( !m_id ) // carefull we're animating
                    {
                        Plasma::Animator::self()->stopCustomAnimation( m_id );
                        m_id = 0;
                    }
                    DragPixmapItem *dragpix = new DragPixmapItem( this );
                    dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                    item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                    dragpix->setPos( m_actualpos, 0 );
                    dragpix->SetClickableUrl( item->urlpage );
                    dragpix->show();
                    
                    int delta = dragpix->boundingRect().width() + m_margin;
                    m_scrollmax += delta;
                    m_actualpos += delta;
                }
                break;
            }
            case PHOTOS_MODE_FADING :
            {
                if ( !m_currentlist.contains( item ) )
                {
                    if ( !m_id ) // carefull we're animating
                    {
                        Plasma::Animator::self()->stopCustomAnimation( m_id );
                        m_id = 0;
                    }
                    DragPixmapItem *dragpix = new DragPixmapItem( this );
                    dragpix->setPixmap( The::svgHandler()->addBordersToPixmap(
                    item->photo->scaledToHeight( (int) size().height() - 4 * m_margin,  Qt::SmoothTransformation ), 5, "", true ) );
                    dragpix->setPos( m_actualpos, 0 );
                    dragpix->SetClickableUrl( item->urlpage );
                    dragpix->show();
                    
                    int delta = dragpix->boundingRect().width() + m_margin;
                    m_scrollmax += delta;
                    m_actualpos += delta;
                }
                break;
            }
        }
    }
    m_currentlist = list;
}

void PhotosScrollWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
//    DEBUG_BLOCK
    Plasma::Animator::self()->stopCustomAnimation( m_id );
    m_id = 0;
}

void PhotosScrollWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
 //   DEBUG_BLOCK
    m_speed = ( event->pos().x() - ( size().width() / 2 ) ) / size().width();
    m_speed*=20;
    
    // if m_id = 0, we don't have an animator yet, let's start the animation
    if ( !m_id )
    {
        m_id = Plasma::Animator::self()->customAnimation( m_scrollmax / 2, m_scrollmax*10, Plasma::Animator::LinearCurve, this, "animate" );
    }
}

void PhotosScrollWidget::animate( qreal anim )
{
    Q_UNUSED( anim );

    // If we're are near the border and still asking to go higher !
    if ( ( ( this->childItems().first()->pos().x() + this->childItems().first()->boundingRect().width() + 10 ) > this->boundingRect().width() ) && ( m_speed < 0 ) )
    {
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
        return;
    }
    // If we're are near the border and still asking to go down
    if ( ( ( this->childItems().last()->pos().x() - 10 ) < 0 ) && ( m_speed > 0 ) )
    {        
        Plasma::Animator::self()->stopCustomAnimation( m_id );
        m_id = 0;
        return;
    }

    int right = 0;
    foreach( QGraphicsItem *it, this->childItems() )
    {
        qreal x = it->pos().x() - m_speed;
        it->setPos( x, it->pos().y() );
        it->update();
        if ( x > right )
            right = x + it->boundingRect().width() + m_margin;
    }
    m_actualpos = right;
}

#include "PhotosScrollWidget.moc"

