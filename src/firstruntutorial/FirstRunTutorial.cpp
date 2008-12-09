/***************************************************************************
 * copyright     : (C) 2008 Jeff Mitchell <mitchell@kde.org>               *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "FirstRunTutorial.h"

#include "Debug.h"

#include <QTimer>

FirstRunTutorial::FirstRunTutorial( QWidget *parent )
    : QObject( parent )
    , m_parent( parent )
    , m_scene( 0 )
    , m_view( 0 )
    , m_fadeShowTimer()
    , m_fadeHideTimer()
    , m_framesMax( 60 )
    , m_itemSet()
{
}

FirstRunTutorial::~FirstRunTutorial()
{
    m_view->hide();
    delete m_view;
    delete m_scene;
}

void
FirstRunTutorial::initOverlay() //SLOT
{
    DEBUG_BLOCK
    m_scene = new QGraphicsScene( m_parent );
    m_view = new QGraphicsView( m_scene, m_parent );
    m_scene->setSceneRect( QRectF( m_parent->rect() ) );
    m_view->setFixedSize( m_parent->size() );
    m_view->setLineWidth( 0 );
    m_view->setFrameStyle( QFrame::NoFrame );
    m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setBackgroundRole( QPalette::Window );
    m_view->setAutoFillBackground( true );

    QColor color = Qt::blue;
    color.setAlpha( 0 );
    QPalette p = m_view->palette();
    p.setColor( QPalette::Window, color );
    m_view->setPalette( p );
    m_view->show();
    m_fadeShowTimer.setDuration( 1000 );
    m_fadeShowTimer.setCurrentTime( 0 );
    m_fadeShowTimer.setFrameRange( 0, 60 );
    m_fadeShowTimer.setCurveShape( QTimeLine::EaseInCurve );
    m_fadeHideTimer.setDuration( 1000 );
    m_fadeHideTimer.setCurrentTime( 0 );
    m_fadeHideTimer.setFrameRange( 0, 60 );
    m_fadeHideTimer.setCurveShape( QTimeLine::EaseOutCurve );
    connect( &m_fadeShowTimer, SIGNAL( frameChanged(int) ), this, SLOT( fadeShowTimerFrameChanged(int) ) );
    connect( &m_fadeHideTimer, SIGNAL( frameChanged(int) ), this, SLOT( fadeHideTimerFrameChanged(int) ) );
    connect( &m_fadeShowTimer, SIGNAL( finished() ), this, SLOT( fadeShowTimerFinished() ) );
    connect( &m_fadeHideTimer, SIGNAL( finished() ), this, SLOT( fadeHideTimerFinished() ) );
    m_fadeShowTimer.start();
}

void
FirstRunTutorial::fadeShowTimerFrameChanged( int frame ) //SLOT
{
    DEBUG_BLOCK
    if( m_fadeShowTimer.state() == QTimeLine::Running )
    {
        qreal val = ( frame * 1.0 ) / m_framesMax;
        QColor color = Qt::blue;
        color.setAlpha( (int)( val * 48 ) );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
    }
}

void
FirstRunTutorial::fadeShowTimerFinished() //SLOT
{
    DEBUG_BLOCK
    QColor color = Qt::blue;
    color.setAlpha( 48 );
    QPalette p = m_view->palette();
    p.setColor( QPalette::Window, color );
    m_view->setPalette( p );
    QTimer::singleShot( 2000, this, SLOT( setupPerms() ) );
}

void
FirstRunTutorial::fadeHideTimerFrameChanged( int frame ) //SLOT
{
    DEBUG_BLOCK
    if( m_fadeHideTimer.state() == QTimeLine::Running )
    {
        qreal val = ( frame * 1.0 ) / m_framesMax;
        QColor color = Qt::blue;
        color.setAlpha( 48 - (int)( val * 48 ) );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
    }
}

void
FirstRunTutorial::fadeHideTimerFinished() //SLOT
{
    DEBUG_BLOCK
    QColor color = Qt::blue;
    color.setAlpha( 0 );
    QPalette p = m_view->palette();
    p.setColor( QPalette::Window, color );
    m_view->setPalette( p );
    deleteLater();
}

void FirstRunTutorial::setupPerms() //SLOT
{
    //Set up permanent items here, like close button, next/prev...for now just cause it to exit
    m_fadeHideTimer.start();
    //now start the first page...see below
}

/*
Here's my thought...have a member variable that controls the "page number" and have a slot that
takes clicks from prev/next buttons (perhaps QPushButtons as QGraphicsWidgets) and changes the page
Each page simply displays its own set of icons/text

If you notice the member QSet, the idea was that when each page is being created, put the items in the QSet
and trigger a common single slot to do the fade in and another to do the fade out (or other animtions)
where it just operates on the items currently in the set...reusability++

*/

#include "FirstRunTutorial.moc"

