/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "CompoundProgressBar.h"

#include "core/support/Debug.h"

#include <QMutexLocker>

#include <KLocalizedString>


CompoundProgressBar::CompoundProgressBar( QWidget *parent )
        : ProgressBar( parent )
{
    m_progressDetailsWidget = new PopupWidget();
    m_progressDetailsWidget->hide();

    connect( cancelButton(), &QAbstractButton::clicked, this, &CompoundProgressBar::cancelAll );
}

CompoundProgressBar::~CompoundProgressBar()
{
    delete m_progressDetailsWidget;
    m_progressDetailsWidget = nullptr;
}

void CompoundProgressBar::addProgressBar( ProgressBar *childBar, QObject *owner )
{
    QMutexLocker locker( &m_mutex );

    m_progressMap.insert( owner, childBar );
    m_progressDetailsWidget->layout()->addWidget( childBar );
    if( m_progressDetailsWidget->width() < childBar->width() )
        m_progressDetailsWidget->setMinimumWidth( childBar->width() );

    m_progressDetailsWidget->setMinimumHeight( childBar->height() * m_progressMap.count()  + 8 );

    m_progressDetailsWidget->reposition();

    connect( childBar, &ProgressBar::percentageChanged,
             this, &CompoundProgressBar::childPercentageChanged );
    connect( childBar, &ProgressBar::cancelled,
             this, &CompoundProgressBar::childBarCancelled );
    connect( childBar, &ProgressBar::complete,
             this, &CompoundProgressBar::childBarComplete );
    connect( owner, &QObject::destroyed, this, &CompoundProgressBar::slotObjectDestroyed );

    if( m_progressMap.count() == 1 )
    {
        setDescription( childBar->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
    }
    else
    {
        setDescription( i18n( "Multiple background tasks running (click to show)" ) );
        cancelButton()->setToolTip( i18n( "Abort all background tasks" ) );
    }

    cancelButton()->setHidden( false );
}

void CompoundProgressBar::endProgressOperation( QObject *owner )
{
    QMutexLocker locker( &m_mutex );

    if( !m_progressMap.contains( owner ) )
        return ;

    childBarComplete( m_progressMap.value( owner ) );
}

void
CompoundProgressBar::slotIncrementProgress()
{
    incrementProgress( sender() );
}

void CompoundProgressBar::incrementProgress( const QObject *owner )
{
    QMutexLocker locker( &m_mutex );

    if( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setValue( m_progressMap.value( owner )->value() + 1 );
}

void CompoundProgressBar::setProgress( const QObject *owner, int steps )
{
    QMutexLocker locker( &m_mutex );

    if( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setValue( steps );
}

void
CompoundProgressBar::mousePressEvent( QMouseEvent *event )
{
    QMutexLocker locker( &m_mutex );

    if( m_progressDetailsWidget->isHidden() )
    {
        if( m_progressMap.count() )
            showDetails();
    }
    else
    {
        hideDetails();
    }

    event->accept();
}

void CompoundProgressBar::setProgressTotalSteps( const QObject *owner, int value )
{
    QMutexLocker locker( &m_mutex );

    if( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setMaximum( value );
}

void CompoundProgressBar::setProgressStatus( const QObject *owner, const QString &text )
{
    QMutexLocker locker( &m_mutex );

    if( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setDescription( text );
}

void CompoundProgressBar::childPercentageChanged()
{
    progressBar()->setValue( calcCompoundPercentage() );
}

void CompoundProgressBar::childBarCancelled( ProgressBar *childBar )
{
    childBarFinished( childBar );
}

void CompoundProgressBar::childBarComplete( ProgressBar *childBar )
{
    childBarFinished( childBar );
}

void CompoundProgressBar::slotObjectDestroyed( QObject *object )
{
    QMutexLocker locker( &m_mutex );

    if( m_progressMap.contains( object ) )
    {
        childBarFinished( m_progressMap.value( object ) );
    }
}

void CompoundProgressBar::childBarFinished( ProgressBar *bar )
{
    QMutexLocker locker( &m_mutex );

    QObject *owner = const_cast<QObject *>( m_progressMap.key( bar ) );
    owner->disconnect( this );
    owner->disconnect( bar );
    m_progressMap.remove( owner );
    m_progressDetailsWidget->layout()->removeWidget( bar );
    m_progressDetailsWidget->setFixedHeight( bar->height() * m_progressMap.count() + 8 );
    m_progressDetailsWidget->reposition();
    delete bar;

    if( m_progressMap.count() == 1 )
    {
        //only one job still running, so no need to use the details widget any more.
        //Also set the text to the description of
        //the job instead of the "Multiple background tasks running" text.
        setDescription( m_progressMap.values().at( 0 )->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
        hideDetails();
    }
    else if( m_progressMap.empty() )
    {
        progressBar()->setValue( 0 );
        hideDetails();
        Q_EMIT( allDone() );
        return;
    }
    else
    {
        setDescription( i18n( "Multiple background tasks running (click to show)" ) );
        cancelButton()->setToolTip( i18n( "Abort all background tasks" ) );
    }

    progressBar()->setValue( calcCompoundPercentage() );
}

int CompoundProgressBar::calcCompoundPercentage()
{
    QMutexLocker locker( &m_mutex );

    int count = m_progressMap.count();
    int total = 0;

    for( ProgressBar *currentBar : m_progressMap )
        total += currentBar->percentage();

    return count == 0 ? 0 : total / count;
}

void CompoundProgressBar::cancelAll()
{
    QMutexLocker locker( &m_mutex );

    for( ProgressBar *currentBar : m_progressMap )
        currentBar->cancel();
}

void CompoundProgressBar::showDetails()
{
    QMutexLocker locker( &m_mutex );

    m_progressDetailsWidget->raise();

    //Hack to make sure it has the right height first time it is shown...
    m_progressDetailsWidget->setFixedHeight(
                m_progressMap.values().at( 0 )->height() * m_progressMap.count() + 8 );
    m_progressDetailsWidget->reposition();
    m_progressDetailsWidget->show();
}

void CompoundProgressBar::hideDetails()
{
    m_progressDetailsWidget->hide();
}

void CompoundProgressBar::toggleDetails()
{
    if( m_progressDetailsWidget->isVisible() )
        hideDetails();
    else
        showDetails();
}
