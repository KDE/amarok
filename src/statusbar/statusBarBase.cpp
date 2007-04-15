/***************************************************************************
 *   Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>          *
 *   Copyright (C) 2005 by Ian Monroe <ian@monroe.nu>                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#define DEBUG_PREFIX "StatusBar"

#include "amarok.h"
#include "debug.h"
#include "statusBarBase.h"
#include "threadmanager.h"

#include <kicon.h>
#include <kiconloader.h>
#include <kjob.h>
#include <klocale.h>
#include <kstandardguiitem.h>
#include <kvbox.h>

#include <QDateTime>      //writeLogFile()
#include <QFile>          //writeLogFile()
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>   //class CloseButton
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>
#include <QToolTip> //QToolTip::palette()


//segregated classes
#include "popupMessage.h"
#include "progressBar.h"


namespace KDE {


namespace SingleShotPool
{
    static void startTimer( int timeout, QObject *receiver, const char *slot )
    {
        QTimer *timer = receiver->findChild<QTimer *>( slot );
        if( !timer ) {
            timer = new QTimer( receiver );
            timer->setObjectName( slot );
            receiver->connect( timer, SIGNAL(timeout()), slot );
        }
        timer->setSingleShot( true );
        timer->start( timeout );
    }

    static inline bool isActive( QObject *parent, const char *slot )
    {
        QTimer *timer = parent->findChild<QTimer*>( slot );

        return timer && timer->metaObject()->className() == "QTimer" && timer->isActive();
    }
}


//TODO allow for uncertain progress periods


StatusBar::StatusBar( QWidget *parent, const char *name )
        : QStatusBar( parent )
        , m_logCounter( -1 )
{
    setObjectName( name );;

    m_mainTextLabel = new QLabel( 0, "mainTextLabel" );
    addPermanentWidget( m_mainTextLabel );
    QToolButton *shortLongButton = new QToolButton( 0 );
    shortLongButton->setObjectName( "shortLongButton" );
    shortLongButton->hide();
    addWidget(shortLongButton);

    KHBox *mainProgressBarBox = new KHBox( this );
    mainProgressBarBox->setObjectName( "progressBox" );
    QToolButton *b1 = new QToolButton( mainProgressBarBox ); //cancelbutton
    b1->setObjectName( "cancelButton" );
    m_mainProgressBar  = new QProgressBar( mainProgressBarBox);
    QToolButton *b2 = new QToolButton( mainProgressBarBox ); //showprogressdetails button
    b2->setObjectName( "showAllProgressDetails" );
    mainProgressBarBox->setSpacing( 2 );
    mainProgressBarBox->hide();
    addWidget(mainProgressBarBox);

    shortLongButton->setIcon( KIcon( "edit_add" ) );
    shortLongButton->setToolTip( i18n( "Show details" ) );
    connect( shortLongButton, SIGNAL(clicked()), SLOT(showShortLongDetails()) );

    b1->setIcon( KIcon( "cancel" ) );
    b2->setIcon( KIcon( "2uparrow") );
    b2->setCheckable( true );
    b1->setToolTip( i18n( "Abort all background-operations" ) );
    b2->setToolTip( i18n( "Show progress detail" ) );
    connect( b1, SIGNAL(clicked()), SLOT(abortAllProgressOperations()) );
    connect( b2, SIGNAL(toggled( bool )), SLOT(toggleProgressWindow( bool )) );

    m_popupProgress = new OverlayWidget( this, mainProgressBarBox, "popupProgress" );
    QStatusBar::addWidget( m_popupProgress );
    m_popupProgress->setFrameStyle( QFrame::Panel | QFrame::Raised );
    m_popupProgress->setFrameShape( QFrame::StyledPanel );
    m_popupProgress->setSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum );
    QGridLayout *gl = new QGridLayout( m_popupProgress );
    gl->setAutoAdd( true );
    gl->setMargin( 6 );
    gl->setSpacing( 3 );
}

/// reimplemented functions

void
StatusBar::polish()
{
    QWidget::ensurePolished();

    int h = 0;
    QObjectList list = queryList( "QWidget", 0, false, false );

    for( QObjectList::iterator it = list.begin(); it != list.end(); it++ ) {
        QObject *o = *it;
        int _h = static_cast<QWidget*>( o ) ->minimumSizeHint().height();
        if ( _h > h )
            h = _h;

//         debug() << o->className() << ", " << o->name() << ": " << _h << ": " << static_cast<QWidget*>(o)->minimumHeight() << endl;

        if ( o->inherits( "QLabel" ) )
            static_cast<QLabel*>(o)->setIndent( 4 );
    }

    h -= 4; // it's too big usually

    for( QObjectList::iterator it = list.begin(); it != list.end(); it++ ) {
        QObject *o = *it;
        static_cast<QWidget*>(o)->setFixedHeight( h );
    }
}

void
StatusBar::paintEvent( QPaintEvent* )
{
    QObjectList list = queryList( "QWidget", 0, false, false );
    QPainter p( this );

    for( QObjectList::iterator it = list.begin(); it != list.end(); it++ ) {
        QObject *o = *it;
        QWidget *w = static_cast<QWidget*>( o );

        if ( !w->isVisible() )
            continue;
        QStyleOption option( 1, QStyleOption::SO_Default );
        style()->drawPrimitive(
                QStyle::PE_FrameStatusBar,
                &option,
                &p);
    }
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
    SHOULD_BE_GUI

    m_mainText = text;

    // it may not be appropriate for us to set the mainText yet
    resetMainText();
}

void
StatusBar::shortMessage( const QString &text, bool longShort )
{
    SHOULD_BE_GUI

    m_mainTextLabel->setText( text );
    m_mainTextLabel->setPalette( QToolTip::palette() );

    SingleShotPool::startTimer( longShort ? 8000 : 5000, this, SLOT(resetMainText()) );

    writeLogFile( text );
}

void
StatusBar::resetMainText()
{
//     if( sender() )
//         debug() << sender()->name() << endl;

    // don't reset if we are showing a shortMessage
    if( SingleShotPool::isActive( this, SLOT(resetMainText()) ) )
        return;

    m_mainTextLabel->setPalette( QPalette() );
    shortLongButton()->hide();

    if( allDone() )
        m_mainTextLabel->setText( m_mainText );

    else {
        ProgressBar *bar = 0;
        uint count = 0;
        oldForeachType( ProgressMap, m_progressMap )
            if( !(*it)->m_done ) {
                bar = *it;
                count++;
            }

        if( count == 1 )
            m_mainTextLabel->setText( bar->description() + i18n("...") );
        else
            m_mainTextLabel->setText( i18n("Multiple background-tasks running") );
    }
}

void
StatusBar::shortLongMessage( const QString &_short, const QString &_long, int type )
{
    SHOULD_BE_GUI

    m_shortLongType = type;

    if( !_short.isEmpty() )
        shortMessage( _short, true );

    if ( !_long.isEmpty() ) {
        m_shortLongText = _long;
        shortLongButton()->show();
        writeLogFile( _long );
    }
}

void
StatusBar::longMessage( const QString &text, int type )
{
    SHOULD_BE_GUI

    if( text.isEmpty() )
        return;

    PopupMessage *message;
    message = new PopupMessage( this, m_mainTextLabel );
    connect( message, SIGNAL(destroyed(QObject *)), this, SLOT(popupDeleted(QObject *)) );
    message->setText( text );

    QString image;

    switch( type )
    {
        case Information:
        case Question:
            image = KIconLoader::global()->iconPath( "messagebox_info", -K3Icon::SizeHuge );
            break;

        case Sorry:
        case Warning:
            image = KIconLoader::global()->iconPath( "messagebox_warning", -K3Icon::SizeHuge );
            break;

        case Error:
            image = KIconLoader::global()->iconPath( "messagebox_critical", -K3Icon::SizeHuge );
            // don't hide error messages.
//             message->setTimeout( 0 );
            break;
    }

    if( !image.isEmpty() )
        message->setImage( image );

    if ( !m_messageQueue.isEmpty() )
         message->stackUnder( m_messageQueue.last() );

    message->display();

    raise();

    m_messageQueue += message;

    writeLogFile( text );
}

void
StatusBar::popupDeleted( QObject *obj )
{
    m_messageQueue.remove( static_cast<QWidget*>( obj ) );
}

void
StatusBar::longMessageThreadSafe( const QString &text, int /*type*/ )
{
    QCustomEvent * e = new QCustomEvent( 1000 );
    e->setData( new QString( text ) );
    QApplication::postEvent( this, e );
}

void
StatusBar::customEvent( QEvent *event )
{
    if( QCustomEvent *e = dynamic_cast<QCustomEvent*>( event ) ) {
        QString *s = static_cast<QString*>( e->data() );
        if( e->type() == 1000 )
            longMessage( *s );
        else if( e->type() == 1001 )
            shortMessage( *s );
        delete s;
    }
}


/// application wide progress monitor

inline bool
StatusBar::allDone()
{
    for( ProgressMap::Iterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ++it )
        if( (*it)->m_done == false )
            return false;

    return true;
}

ProgressBar&
StatusBar::newProgressOperationInternal( QObject *owner )
{
    SHOULD_BE_GUI

    if ( m_progressMap.contains( owner ) )
        return *m_progressMap[owner];

    if( allDone() )
        // if we're allDone then we need to remove the old progressBars before
        // we start anything new or the total progress will not be accurate
        pruneProgressBars();
    else
        toggleProgressWindowButton()->show();
    QLabel *label = new QLabel( m_popupProgress );
    m_progressMap.insert( owner, new ProgressBar( m_popupProgress, label ) );

    m_popupProgress->reposition();

    //connect( owner, SIGNAL(destroyed( QObject* )), SLOT(endProgressOperation( QObject* )), Qt::DirectConnection );

    // so we can show the correct progress information
    // after the ProgressBar is setup
    SingleShotPool::startTimer( 0, this, SLOT(updateProgressAppearance()) );

    progressBox()->show();
    cancelButton()->setEnabled( true );

    return *m_progressMap[ owner ];
}

void StatusBar::shortMessageThreadSafe( const QString &text )
{
    QCustomEvent *e = new QCustomEvent( 1001 );
    e->setData( new QString( text ) );
    QApplication::postEvent( this, e );
}

ProgressBar&
StatusBar::newProgressOperation( QObject *owner )
{
    connect( owner, SIGNAL(destroyed( QObject* )), SLOT(endProgressOperation( QObject* )) );
    return newProgressOperationInternal( owner );
}


ProgressBar&
StatusBar::newProgressOperation( KJob *job )
{
    SHOULD_BE_GUI

    ProgressBar & bar = newProgressOperation( static_cast<QObject*>( job ) );
    bar.setMinimum( 0 );
    bar.setMaximum( 100 );

    if(!allDone())
        toggleProgressWindowButton()->show();
    connect( job, SIGNAL(result( KJob* )), SLOT(endProgressOperation()) );
    //TODO connect( job, SIGNAL(infoMessage( KIO::Job *job, const QString& )), SLOT() );
    connect( job, SIGNAL(percent( KJob*, unsigned long )), SLOT(setProgress( KJob*, unsigned long )) );

    return bar;
}

void
StatusBar::endProgressOperation()
{
    QObject *owner = const_cast<QObject*>( sender() ); //HACK deconsting it
    KJob *job = dynamic_cast<KJob*>( owner );

    //FIXME doesn't seem to work for KIO::DeleteJob, it has it's own error handler and returns no error too
    // if you try to delete http urls for instance <- KDE SUCKS!

    if( job && job->error() )
        shortLongMessage( QString(), job->errorString(), Error );

    endProgressOperation( owner );
}

void
StatusBar::endProgressOperation( QObject *owner )
{
    //the owner of this progress operation has been deleted
    //we need to stop listening for progress from it
    //NOTE we don't delete it yet, as this upsets some
    //things, we just call setDone().

    if ( !m_progressMap.contains( owner ) )
    {
        SingleShotPool::startTimer( 2000, this, SLOT(hideMainProgressBar()) );
        return ;
    }

    m_progressMap[owner]->setDone();

    if( allDone() && m_popupProgress->isHidden() ) {
        cancelButton()->setEnabled( false );
        SingleShotPool::startTimer( 2000, this, SLOT(hideMainProgressBar()) );
    }

    updateTotalProgress();
}

void
StatusBar::abortAllProgressOperations() //slot
{
    for( ProgressMap::Iterator it = m_progressMap.begin(), end = m_progressMap.end(); it != end; ++it )
        (*it)->m_abort->animateClick();

    m_mainTextLabel->setText( i18n("Aborting all jobs...") );

    cancelButton()->setEnabled( false );
}

void
StatusBar::toggleProgressWindow( bool show ) //slot
{
    m_popupProgress->reposition(); //FIXME shouldn't be needed, adding bars doesn't seem to do this
    m_popupProgress->setVisible( show );

    if( !show )
        SingleShotPool::startTimer( 2000, this, SLOT(hideMainProgressBar()) );
}

void
StatusBar::showShortLongDetails()
{
    if( !m_shortLongText.isEmpty() )
        longMessage( m_shortLongText, m_shortLongType );

    m_shortLongType = Information;
    m_shortLongText.clear();
    shortLongButton()->hide();
}

void
StatusBar::showMainProgressBar()
{
    if( !allDone() )
        progressBox()->show();
}

void
StatusBar::hideMainProgressBar()
{
    if( allDone() && m_popupProgress->isHidden() )
    {
        pruneProgressBars();

        resetMainText();

        m_mainProgressBar->setValue( 0 );
        progressBox()->hide();
    }
}

void
StatusBar::setProgress( int steps )
{
    setProgress( sender(), steps );
}

void
StatusBar::setProgress( KJob *job, unsigned long percent )
{
    setProgress( static_cast<QObject*>( job ), percent );
}

void
StatusBar::setProgress( const QObject *owner, int steps )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[ owner ] ->setValue( steps );

    updateTotalProgress();
}

void
StatusBar::incrementProgressTotalSteps( const QObject *owner, int inc )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap[ owner ] ->setMaximum( m_progressMap[ owner ] ->maximum() + inc );

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
        return;

    m_progressMap[owner]->setValue( m_progressMap[ owner ] ->value() + 1 );

    updateTotalProgress();
}

void
StatusBar::updateTotalProgress()
{
    uint totalSteps = 0;
    uint progress = 0;

    foreach( ProgressBar *it, m_progressMap ) {
        totalSteps += it->maximum();
        progress += it->value();
    }

    if( totalSteps == 0 && progress == 0 )
        return;

    m_mainProgressBar->setMaximum( totalSteps );
    m_mainProgressBar->setValue( progress );

    pruneProgressBars();
}

void
StatusBar::updateProgressAppearance()
{
    toggleProgressWindowButton()->setVisible( m_progressMap.count() > 1 );

    resetMainText();

    updateTotalProgress();
}

void
StatusBar::pruneProgressBars()
{
    ProgressMap::Iterator it = m_progressMap.begin();
    const ProgressMap::Iterator end = m_progressMap.end();
    int count = 0;
    bool removedBar = false;
    while( it != end )
        if( (*it)->m_done == true ) {
            delete (*it)->m_label;
            delete (*it)->m_abort;
            delete (*it);

            ProgressMap::Iterator jt = it;
            ++it;
            m_progressMap.erase( jt );
            removedBar = true;
        }
        else {
            ++it;
            ++count;
        }
    if(count==1 && removedBar) //if its gone from 2 or more bars to one bar...
    {
        resetMainText();
        toggleProgressWindowButton()->hide();
        m_popupProgress->setVisible(false);
    }
}

/// Method which writes to a rotating log file.
void
StatusBar::writeLogFile( const QString &text )
{
    if( text.isEmpty() ) return;

    const int counter = 4; // number of logs to keep
    const uint maxSize = 30000; // approximately 1000 lines per log file
    int c = counter;
    QString logBase = Amarok::saveLocation() + "statusbar.log.";
    QFile file;

    if( m_logCounter < 0 ) //find which log to write to
    {
        for( ; c > 0; c-- )
        {
            QString log = logBase + QString::number(c);
            file.setFileName( log );

            if( QFile::exists( log ) && file.size() <= maxSize )
                break;
        }
        if( c == 0 ) file.setFileName( logBase + '0' );
        m_logCounter = c;
    }
    else
    {
        file.setFileName( logBase + QString::number(m_logCounter) );
    }

    if( file.size() > maxSize )
    {
        m_logCounter++;
        m_logCounter = m_logCounter % counter;

        file.setFileName( logBase + QString::number(m_logCounter) );
        // if we have overflown the log, then we want to overwrite the previous content
        if( !file.open( QIODevice::WriteOnly ) ) return;
    }
    else if( !file.open( QIODevice::WriteOnly|QIODevice::Append ) ) return;

    QTextStream stream( &file );
    stream.setCodec( "UTF8" );

    stream << "[" << KGlobal::locale()->formatDateTime( QDateTime::currentDateTime() ) << "] " << text << endl;
}

} //namespace KDE


#include "statusBarBase.moc"
