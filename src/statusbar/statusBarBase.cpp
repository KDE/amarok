/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define DEBUG_PREFIX "StatusBar"

#include "debug.h"
#include <kio/job.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <ksqueezedtextlabel.h>
#include "overlayWidget.h"
#include <qapplication.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmessagebox.h>
#include <qobjectlist.h> //polish()
#include <qpainter.h>
#include <qpalette.h>
#include <qprogressbar.h>
#include <qstyle.h>   //class CloseButton
#include <qtimer.h>
#include <qtoolbutton.h>
#include <qtooltip.h> //QToolTip::palette()
#include <qvbox.h>
#include "statusBarBase.h"

//segregated classes
#include "popupMessage.h"
#include "popupMessage.moc"
#include "progressBar.h"


namespace KDE {


//TODO allow for uncertain progress periods


StatusBar::StatusBar( QWidget *parent, const char *name )
        : QFrame( parent, name )
        , m_tempMessageTimer( new QTimer( this ) )
{
    //we need extra spacing due to the way we paint the surrounding boxes
    QBoxLayout *layout = new QHBoxLayout( this, /*margin*/2, /*spacing*/5 );
    layout->setAutoAdd( true );

    m_mainTextLabel = new QLabel( this, "mainTextLabel" );
    m_mainTextLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    QHBox *mainProgressBarBox = new QHBox( this, "progressBox" );
    QToolButton *b1 = new QToolButton( mainProgressBarBox, "cancelButton" );
    m_mainProgressBar  = new QProgressBar( mainProgressBarBox, "mainProgressBar" );
    QToolButton *b2 = new QToolButton( mainProgressBarBox, "showAllProgressDetails" );
    mainProgressBarBox->setSpacing( 2 );
    mainProgressBarBox->hide();

    b1->setIconSet( SmallIconSet( "cancel" ) );
    b2->setIconSet( SmallIconSet( "2uparrow") );
    b2->setToggleButton( true );
    QToolTip::add( b1, i18n( "Abort all background-operations" ) );
    QToolTip::add( b2, i18n( "Show progress detail" ) );
    connect( b1, SIGNAL(clicked()), SLOT(abortAllProgressOperations()) );
    connect( b2, SIGNAL(toggled( bool )), SLOT(toggleProgressWindow( bool )) );

    m_popupProgress = new OverlayWidget( this, mainProgressBarBox, "popupProgress" );
    m_popupProgress->setMargin( 1 );
    m_popupProgress->setFrameStyle( QFrame::Box | QFrame::Raised );
    m_popupProgress->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
   (new QGridLayout( m_popupProgress, 1 /*rows*/, 3 /*cols*/, 6, 3 ))->setAutoAdd( true );

    connect( m_tempMessageTimer, SIGNAL(timeout()), SLOT(resetMainText()) );
}


/// reimplemented functions

void
StatusBar::polish()
{
    QFrame::polish();

    int h = 0;
    QObjectList *list = queryList( "QWidget", 0, false, false );

    for( QObject * o = list->first(); o; o = list->next() ) {
        int _h = static_cast<QWidget*>( o ) ->minimumSizeHint().height();
        if ( _h > h )
            h = _h;

        debug() << o->className() << ", " << o->name() << ": " << _h << ": " << static_cast<QWidget*>(o)->minimumHeight() << endl;

        if ( o->inherits( "QLabel" ) )
            static_cast<QLabel*>(o)->setIndent( 4 );
    }

    for ( QObject * o = list->first(); o; o = list->next() )
        static_cast<QWidget*>( o ) ->setFixedHeight( h );

    delete list;
}

void
StatusBar::paintEvent( QPaintEvent* )
{
    QObjectList *list = queryList( "QWidget", 0, false, false );
    QPainter p( this );

    for( QObject * o = list->first(); o; o = list->next() ) {
        QWidget *w = (QWidget*)o;

        if ( !w->isVisible() )
            continue;

        style().drawPrimitive(
                QStyle::PE_StatusBarSection,
                &p,
                QRect( w->x() - 1, w->y() - 1, w->width() + 2, w->height() + 2 ),
                colorGroup(),
                QStyle::Style_Default,
                QStyleOption( w ) );
    }

    delete list;
}

bool
StatusBar::event( QEvent *e )
{
    if ( e->type() == QEvent::LayoutHint )
        update();

    return QWidget::event( e );
}


/// Messaging system

void
StatusBar::setMainText( const QString &text )
{
    m_mainText = text;

    if ( !m_tempMessageTimer->isActive() )
        // if we're showing a shortMessage, resetMainText() will be
        // called within 5 seconds
        m_mainTextLabel->setText( text );
}

void
StatusBar::shortMessage( const QString &text )
{
    m_mainTextLabel->setText( text );
    m_mainTextLabel->setPalette( QToolTip::palette() );

    m_tempMessageTimer->start( 5000, true );
}

void
StatusBar::resetMainText()
{
    m_mainTextLabel->unsetPalette();
    m_mainTextLabel->setText( m_mainText );
}

void
StatusBar::shortLongMessage( const QString &_short, const QString &_long )
{
    shortMessage( _short );

    if ( !_long.isEmpty() ) {
        AMAROK_NOTIMPLEMENTED
    }
}

void
StatusBar::longMessage( const QString &text, int type )
{
    PopupMessage * message;
    message = new PopupMessage( this, m_mainTextLabel, "popupLabel" );
    message->setText( text );

    if ( m_messageQueue.isEmpty() )
        message->animate();
    else {
        message->stackUnder( m_messageQueue.last() );
        message->show();
        message->move( 0, this->y() - message->height() );
    }

    raise();

    m_messageQueue += message;
}

void
StatusBar::longMessageThreadSafe( const QString &text, int type )
{
    QCustomEvent * e = new QCustomEvent( 1000 );
    e->setData( new QString( text ) );
    QApplication::postEvent( this, e );
}

void
StatusBar::customEvent( QCustomEvent *e )
{
    QString * s = static_cast<QString*>( e->data() );
    longMessage( *s );
    delete s;
}


/// application wide progress monitor

ProgressBar&
StatusBar::newProgressOperation( QObject *owner )
{
    if ( m_progressMap.contains( owner ) )
        return *m_progressMap[owner];

    QLabel * label = new QLabel( m_popupProgress );
    m_progressMap.insert( owner, new ProgressBar( m_popupProgress, label ) );

    connect( owner, SIGNAL(destroyed( QObject* )), SLOT(endProgressOperation( QObject* )) );

    updateTotalProgress();

    progressBox()->show();
    cancelButton()->setEnabled( true );

    return *m_progressMap[ owner ];
}

ProgressBar&
StatusBar::newProgressOperation( KIO::Job *job )
{
    ProgressBar & bar = newProgressOperation( ( QObject* ) job );
    bar.setTotalSteps( 100 );

    connect( job, SIGNAL(result( KIO::Job* )), SLOT(endProgressOperation()) );
    //TODO connect( job, SIGNAL(infoMessage( KIO::Job *job, const QString& )), SLOT() );
    connect( job, SIGNAL(percent( KIO::Job*, unsigned long )), SLOT(setProgress( KIO::Job*, unsigned long )) );

    return bar;
}

void
StatusBar::endProgressOperation()
{
    QObject * owner = ( QObject* ) sender(); //HACK deconsting it
    KIO::Job *job = dynamic_cast<KIO::Job*>( owner );

    //FIXME doesn't seem to work for KIO::DeleteJob, it has it's own error handler and returns no error too
    // if you try to delete http urls for instance <- KDE SUCKS!

    if ( job && job->error() )
        longMessage( job->errorString(), Error );

    endProgressOperation( owner );
}

void
StatusBar::endProgressOperation( QObject *owner )
{
    //the owner of this progress operation has been deleted
    //we need to stop listening for progress from it

    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[owner]->setDone();

    bool done = true;
    for( ProgressMap::Iterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ++it )
        if ( ( *it ) ->m_done == false ) {
            done = false;
            break;
        }

    if ( done && !m_popupProgress->isShown() ) {
        //FIXME we need to be able to stop this
        cancelButton()->setEnabled( false );
        QTimer::singleShot( 2000, this, SLOT(hideMainProgressBar()) );
    }

    updateTotalProgress();
}

void
StatusBar::abortAllProgressOperations() //slot
{
    for( ProgressMap::Iterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ++it )
        (*it)->m_abort->animateClick();

    cancelButton()->setEnabled( false );
}

void
StatusBar::toggleProgressWindow( bool show ) //slot
{
    //TODO delete old progressBars

    //TODO a foreach for this
    for ( ProgressMap::Iterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ) {
        if ( ( *it ) ->m_done == true ) {

            delete (*it)->m_label;
            delete (*it)->m_abort;
            delete (*it);

            ProgressMap::Iterator jt = it;
            ++it;
            m_progressMap.erase( jt );
        } else
            ++it;
    }

    m_popupProgress->adjustSize(); //FIXME shouldn't be needed, adding bars doesn't seem to do this
    m_popupProgress->setShown( show );

    if ( !show && m_progressMap.isEmpty() )
        QTimer::singleShot( 2000, this, SLOT(hideMainProgressBar()) );
}

void
StatusBar::hideMainProgressBar()
{
    if( !m_popupProgress->isShown() ) {
        m_mainProgressBar->setProgress( 0 );
        progressBox()->hide();
    }
}

void
StatusBar::setProgress( int steps )
{
    setProgress( sender(), steps );
}

void
StatusBar::setProgress( KIO::Job *job, unsigned long percent )
{
    setProgress( ( QObject* ) job, percent );
}

void
StatusBar::setProgress( const QObject *owner, int steps )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[ owner ] ->setProgress( steps );

    updateTotalProgress();
}

void
StatusBar::setProgressStatus( const QObject *owner, const QString &text )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[owner]->setStatus( text );
}

void StatusBar::incrementProgress()
{
    incrementProgress( sender() );
}

void
StatusBar::incrementProgress( const QObject *owner )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[owner]->setProgress( m_progressMap[ owner ] ->progress() + 1 );

    updateTotalProgress();
}

void
StatusBar::updateTotalProgress()
{
    uint totalSteps = 0;
    uint progress = 0;

    for( ProgressMap::ConstIterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ++it ) {
        if ( !(*it)->m_done ) {
            totalSteps += (*it)->totalSteps();
            progress += (*it)->progress();
        }
    }

    if ( totalSteps == 0 && progress == 0 )
        return;

    m_mainProgressBar->setTotalSteps( totalSteps );
    m_mainProgressBar->setProgress( progress );
}

} //namespace KDE


#include "statusBarBase.moc"
