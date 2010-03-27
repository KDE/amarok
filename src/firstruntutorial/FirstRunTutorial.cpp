/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
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

#include "FirstRunTutorial.h"

#include "core/support/Debug.h"
#include "FirstRunTutorialPage.h"
#include "MainWindow.h"

#include <QChar>
#include <QString>
#include <QTimer>

static int MAX_PAGE = 1;
static int TUTORIAL_MARGIN = 200;


FirstRunTutorial::FirstRunTutorial( QWidget *parent )
    : QObject( parent )
    , m_parent( parent )
    , m_scene( 0 )
    , m_view( 0 )
    , m_fadeShowTimer()
    , m_fadeHideTimer()
    , m_framesMax( 60 )
    , m_itemSet()
    , m_pageNum( 0 )
{}

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
    m_view->resize( m_parent->size() );
    m_view->setLineWidth( 0 );
    m_view->setFrameStyle( QFrame::NoFrame );
    m_view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_view->setBackgroundRole( QPalette::Window );
    m_view->setAutoFillBackground( true );

    m_parent->installEventFilter( this );

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
    if( m_fadeShowTimer.state() == QTimeLine::Running && m_pageNum == 0 )
    {
        qreal val = ( frame * 1.0 ) / m_framesMax;
        QColor color = Qt::blue;
        color.setAlpha( (int)( val * 48 ) );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
    }
    else
    {
        #if QT_VERSION >= 0x040500
        m_pages[m_pageNum]->setOpacity( ( frame * 1.0 ) / m_framesMax );
        #endif
    }
}

void
FirstRunTutorial::fadeShowTimerFinished() //SLOT
{
    if( m_pageNum == 0 )
    {
        QColor color = Qt::blue;
        color.setAlpha( 48 );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
        m_pageNum++;
        QTimer::singleShot( 1000, this, SLOT( nextPage() ) );
    }
    else
    {
        #if QT_VERSION >= 0x040500
        m_pages[m_pageNum]->setOpacity( 1.0 );
        #endif
    }
}

void
FirstRunTutorial::fadeHideTimerFrameChanged( int frame ) //SLOT
{
    if( m_fadeHideTimer.state() == QTimeLine::Running && m_pageNum > MAX_PAGE )
    {
        qreal val = ( frame * 1.0 ) / m_framesMax;
        QColor color = Qt::blue;
        color.setAlpha( 48 - (int)( val * 48 ) );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
    }
    else
    {
        #if QT_VERSION >= 0x040500
        m_pages[m_pageNum]->setOpacity( 1 - ( ( frame * 1.0 ) / m_framesMax ) );
        #endif
    }
}

void
FirstRunTutorial::fadeHideTimerFinished() //SLOT
{
    if( m_pageNum > MAX_PAGE )
    {
        QColor color = Qt::blue;
        color.setAlpha( 0 );
        QPalette p = m_view->palette();
        p.setColor( QPalette::Window, color );
        m_view->setPalette( p );
        deleteLater();
    }
    else
    {
        #if QT_VERSION >= 0x040500
        m_pages[m_pageNum]->setOpacity( 0 );
        #endif
        m_pages[m_pageNum]->deleteLater();
        m_pages[m_pageNum] = 0;
        m_pageNum++;
        QTimer::singleShot( 1000, this, SLOT( nextPage() ) );
    }
}

void FirstRunTutorial::nextPage() //SLOT
{
    DEBUG_BLOCK
    debug() << "page is " << m_pageNum << " out of " << MAX_PAGE;
    if( m_pageNum <= MAX_PAGE )
    {
        QString page( QString("1slotPage%1()").arg( m_pageNum ) );
        QTimer::singleShot( 0, this, page.toAscii().constData() );
    }
    else
    {
        m_fadeHideTimer.start();        
    }
}

/*
Here's my thought...have a member variable that controls the "page number" and have a slot that
takes clicks from prev/next buttons (perhaps QPushButtons as QGraphicsWidgets) and changes the page
Each page simply displays its own set of icons/text

If you notice the member QSet, the idea was that when each page is being created, put the items in the QSet
and trigger a common single slot to do the fade in and another to do the fade out (or other animtions)
where it just operates on the items currently in the set...reusability++

*/

void FirstRunTutorial::slotPage1() //SLOT
{
    DEBUG_BLOCK

    FirstRunTutorialPage* page = new FirstRunTutorialPage();
    m_pages[m_pageNum] = page;
    page->setGeometry( The::mainWindow()->frameGeometry().adjusted( TUTORIAL_MARGIN, TUTORIAL_MARGIN, -TUTORIAL_MARGIN, -TUTORIAL_MARGIN ) );
    #if QT_VERSION >= 0x040500
    page->setOpacity( 0 );
    #endif

    m_scene->addItem( page );
    connect( page, SIGNAL( pageClosed() ), &m_fadeHideTimer, SLOT( start() ) );

    m_fadeShowTimer.start();
}


bool FirstRunTutorial::eventFilter( QObject* watched, QEvent* event )
{
    if( watched == m_parent )
    {
        if( event->type() == QEvent::Resize )
        {
            debug() << "View resizeEvent";
            m_pages[1]->triggerResize( m_parent->rect().adjusted( TUTORIAL_MARGIN, TUTORIAL_MARGIN, -TUTORIAL_MARGIN, -TUTORIAL_MARGIN ) );
            return false;
        }
    }

    return QObject::eventFilter( watched, event );
}



#include "FirstRunTutorial.moc"

