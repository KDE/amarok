/***************************************************************************
                          expandbutton.cpp  -  description
                             -------------------
    begin                : Die Feb 4 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "expandbutton.h"

#include <qbitmap.h>
#include <qevent.h>
#include <qobject.h>
#include <qobjectlist.h>
#include <qpaintdevice.h>
#include <qpainter.h>
#include <qpalette.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qpointarray.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qtimer.h>
#include <qwidget.h>
#include <qtooltip.h>

#include <kdebug.h>
#include <klocale.h>
#include <kglobal.h>


ExpandButton::ExpandButton( const QString &text, QWidget *parent, QObject *receiver, const char *slot )
    : QPushButton( text, parent )
    , m_animFlag( ANIM_IDLE )
    , m_expanded( false )
{
    setName( "ExpandButton" );
    setFocusPolicy( QWidget::NoFocus );
    setFlat( true );

    //m_ButtonList = QPtrList<ExpandButton>(); disabled because it will already be initialised in this way on construction
    connect( this, SIGNAL( pressed() ), this,     SLOT( slotDelayExpand() ) );
    connect( this, SIGNAL( clicked() ), receiver, slot );
}


ExpandButton::ExpandButton( const QString &text, ExpandButton *parent, QObject *receiver, const char *slot )
    : QPushButton( text, parent->parentWidget() )
    , m_animFlag( ANIM_IDLE )
    , m_expanded( false )
{
    setName( "ExpandButton_child" );
    setFocusPolicy( QWidget::NoFocus );

    parent->m_ButtonList.append( this );
    hide();

    //FIXME inefficient
    if ( QToolTip::textFor( parent ) != QString::null ) QToolTip::remove( parent );
    QToolTip::add( parent, i18n( "Keep button pressed for sub-menu" ) );

    connect( this, SIGNAL( clicked() ), receiver, slot );
}



ExpandButton::~ExpandButton()
{
}


// METHODS ----------------------------------------------------

void ExpandButton::mouseReleaseEvent( QMouseEvent *e )
{
    if ( m_animFlag == ANIM_EXPAND || m_animFlag == ANIM_SHOW )
    {
        m_animFlag = ANIM_SHRINK;
        m_animAdd = 0;

        if ( hitButton( e->pos() ) && !m_expanded )
            emit clicked();

        for ( unsigned int i = 0; i < m_ButtonList.count(); i++ )
        {
            ExpandButton *child = m_ButtonList.at( i );
            if ( child->hitButton( child->mapFromGlobal( e->globalPos() ) ) )
                child->animateClick();
        }
    }
}


void ExpandButton::mouseMoveEvent( QMouseEvent *e )
{
    for ( unsigned int i = 0; i < m_ButtonList.count(); i++ )
    {
        ExpandButton *child = m_ButtonList.at( i );
        if ( child->hitButton( child->mapFromGlobal( e->globalPos() ) ) )
        {
            child->setDown( true );
        }
        else
        {
            child->setDown( false );
        }
    }
}


// SLOTS ------------------------------------------------------

void ExpandButton::slotDelayExpand()
{
    if ( m_animFlag == ANIM_IDLE )
    {
        QTimer::singleShot( 300, this, SLOT( slotStartExpand() ) );
        m_animFlag = ANIM_EXPAND;
    }
}


void ExpandButton::slotStartExpand()
{
    m_expanded = true;
    m_animSpeed = 0.3;

    m_pSavePixmap = new QPixmap( QPixmap::grabWindow( parentWidget()->winId(),
        x(), y() - m_ButtonList.count() * height(),
        width(), m_ButtonList.count() * height() ) );

    m_pComposePixmap = new QPixmap( m_pSavePixmap->size() );

                                                  // contains all buttons in "up" position
    m_pBlitMap1 = new QPixmap( m_pComposePixmap->size() );
                                                  // contains all buttons in "down" position
    m_pBlitMap2 = new QPixmap( m_pComposePixmap->size() );
    bitBlt( m_pBlitMap1, 0, 0, m_pBlitMap1, 0, 0, -1, -1, Qt::ClearROP );
    bitBlt( m_pBlitMap2, 0, 0, m_pBlitMap2, 0, 0, -1, -1, Qt::ClearROP );

    int yPos = y();
    for ( unsigned int i = 0; i < m_ButtonList.count(); i++ )
    {
        ExpandButton *child = m_ButtonList.at( i );
        child->resize( size() );
        yPos -= child->height();
        child->move( x(), yPos );
        child->setDown( true );
        QPixmap tmp = QPixmap::grabWidget( child );
        bitBlt( m_pBlitMap2, 0, height() * i, &tmp );
        child->setDown( false );
        tmp = QPixmap::grabWidget( child );
        bitBlt( m_pBlitMap1, 0, height() * i, &tmp );
    }

    // FIXME: with the mask the buttons get transparent, which looks cool, but won't work with the Liquid
    //        style, since that's badly b0rked. Fuck Liquid.
//     m_pBlitMap1->setMask( m_pBlitMap1->createHeuristicMask() );
//     m_pBlitMap2->setMask( m_pBlitMap2->createHeuristicMask() );

    m_animHeight = 0; m_animAdd = 0;
    m_pTimer = new QTimer( this );
    connect( m_pTimer, SIGNAL( timeout() ), this, SLOT( slotAnimTimer() ) );
    m_pTimer->start( 20, false );
}


void ExpandButton::slotAnimTimer()
{
    if ( m_animFlag == ANIM_EXPAND )
    {
        m_animHeight += (int) m_animAdd;
        m_animAdd += m_animSpeed;

        if ( m_animHeight >= m_pComposePixmap->height() )
        {
            m_animHeight = m_pComposePixmap->height();
            m_animFlag = ANIM_SHOW;
            return;
        }
    }
    if ( m_animFlag == ANIM_SHRINK )
    {
        m_animHeight -= (int) m_animAdd;
        m_animAdd += m_animSpeed;

        if ( m_animHeight < 0 )
        {
                                                  // Restore painted areas
            bitBlt( parentWidget(), x(), y() - m_pComposePixmap->height(), m_pSavePixmap );

            m_expanded = false;
            setDown( false );
            delete m_pTimer;
            m_pTimer = NULL;
            delete m_pSavePixmap;
            m_pSavePixmap = NULL;
            delete m_pComposePixmap;
            m_pComposePixmap = NULL;
            delete m_pBlitMap1;
            m_pBlitMap1 = NULL;
            delete m_pBlitMap2;
            m_pBlitMap2 = NULL;
            m_animFlag = ANIM_IDLE;

            parentWidget()->update();
            return;
        }
    }

    bitBlt( m_pComposePixmap, 0, 0, m_pSavePixmap );
    int yPos = m_pComposePixmap->height() - m_animHeight;

    for ( int i = (int) m_ButtonList.count() - 1 ; i >= 0; i-- )
    {
        ExpandButton *child = m_ButtonList.at( i );
        int clipY = m_pComposePixmap->height() - yPos;

        if ( clipY > height() )
            clipY = height();
        if ( child->isDown() )
            bitBlt( m_pComposePixmap, 0, yPos, m_pBlitMap2, 0, i * height(), -1, clipY );
        else
            bitBlt( m_pComposePixmap, 0, yPos, m_pBlitMap1, 0, i * height(), -1, clipY );

        yPos += height();
    }
    bitBlt( parentWidget(), x(), y() - m_pComposePixmap->height(), m_pComposePixmap );
}


void ExpandButton::drawButtonLabel( QPainter *p )
{
   QPushButton::drawButtonLabel( p );

   if ( !m_ButtonList.isEmpty() )
   {
      QCOORD size      = KMIN( width(), height() ) / 4;
      QCOORD minAdjust = KMAX( size / 3, 5 );

      QPointArray pa( 3 );
      pa.setPoint( 0, width() - minAdjust - size, minAdjust        );
      pa.setPoint( 1, width() - minAdjust,        minAdjust        );
      pa.setPoint( 2, width() - minAdjust,        minAdjust + size );

      p->setPen( Qt::black );
      p->setBrush( Qt::black );
      p->drawConvexPolygon( pa );
   }
}


#include "expandbutton.moc"
