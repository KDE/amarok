/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "Debug.h"
#include "MainWindow.h"

#include <KIcon>
#include <KLocale>

#include <QLayout>

CompoundProgressBar::CompoundProgressBar( QWidget * parent )
        : ProgressBar( parent )
{
    m_showDetailsButton = new QToolButton( extrabuttonSpace() );
    m_showDetailsButton->setIcon( KIcon( "arrow-up-double-amarok" ) );

    m_progressDetailsWidget = new PopupWidget( parent );
    m_progressDetailsWidget->hide();
    
    connect( m_showDetailsButton, SIGNAL( clicked() ), this, SLOT( toggleDetails() ) );
    connect( cancelButton(), SIGNAL( clicked() ), this, SLOT( cancelAll() ) );
}

CompoundProgressBar::~CompoundProgressBar()
{
    DEBUG_BLOCK

    delete m_progressDetailsWidget;
    m_progressDetailsWidget = 0;
}

void CompoundProgressBar::addProgressBar( ProgressBar * childBar, QObject *owner )
{
    DEBUG_BLOCK

    m_progressMap.insert( owner, childBar );
    m_progressDetailsWidget->layout()->addWidget( childBar );
    if ( m_progressDetailsWidget->width() < childBar->width() )
        m_progressDetailsWidget->setMinimumWidth( childBar->width() );

    m_progressDetailsWidget->setMinimumHeight( childBar->height() * m_progressMap.count()  + 8 );

    m_progressDetailsWidget->reposition();

    connect( childBar, SIGNAL( percentageChanged( int ) ), this, SLOT( childPercentageChanged() ) );
    connect( childBar, SIGNAL( cancelled( ProgressBar * ) ), this, SLOT( childBarCancelled( ProgressBar * ) ) );
    connect( childBar, SIGNAL( complete( ProgressBar * ) ), this, SLOT( childBarComplete( ProgressBar * ) ) );

    if ( m_progressMap.count() == 1 )
    {
        setDescription( childBar->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
    }
    else
    {
        setDescription( i18n( "Multiple background tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background tasks" ) );
    }

    cancelButton()->setEnabled( true );

    handleDetailsButton();
}

void CompoundProgressBar::endProgressOperation( const QObject * owner )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    childBarComplete( m_progressMap.value( owner ) );
}

void CompoundProgressBar::incrementProgress( const QObject * owner )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setValue( m_progressMap.value( owner )->value() + 1 );
}

void CompoundProgressBar::setProgress( const QObject * owner, int steps )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setValue( steps );
}

void CompoundProgressBar::incrementProgressTotalSteps( const QObject * owner, int inc )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setMaximum( m_progressMap.value( owner )->maximum() + inc );
}

void CompoundProgressBar::setProgressStatus( const QObject * owner, const QString & text )
{
    if ( !m_progressMap.contains( owner ) )
        return ;

    m_progressMap.value( owner )->setDescription( text );
}

void CompoundProgressBar::childPercentageChanged()
{
    progressBar()->setValue( calcCompoundPercentage() );
}

void CompoundProgressBar::childBarCancelled( ProgressBar * childBar )
{
    DEBUG_BLOCK

    m_progressMap.remove( m_progressMap.key( childBar ) );
    m_progressDetailsWidget->layout()->removeWidget( childBar );
    m_progressDetailsWidget->setFixedHeight( childBar->height() * m_progressMap.count() + 8 );
    m_progressDetailsWidget->reposition();
    delete childBar;

    if ( m_progressMap.count() == 1 )
    {
        setDescription( m_progressMap.values().at( 0 )->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
    }
    else
    {
        setDescription( i18n( "Multiple background tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background tasks" ) );
    }

    if ( m_progressMap.count() == 0 )
    {
        progressBar()->setValue( 0 );
        m_progressDetailsWidget->setMinimumWidth( 0 );
        cancelButton()->setEnabled( false );
        hideDetails();
        emit( allDone() );
        return;
    }

    progressBar()->setValue( calcCompoundPercentage() );

    handleDetailsButton();
}

void CompoundProgressBar::childBarComplete( ProgressBar * childBar )
{
    DEBUG_BLOCK

    m_progressMap.remove( m_progressMap.key( childBar ) );
    m_progressDetailsWidget->layout()->removeWidget( childBar );
    m_progressDetailsWidget->setFixedHeight( childBar->height()  * m_progressMap.count() + 8 );
    m_progressDetailsWidget->reposition();
    delete childBar;

    if ( m_progressMap.count() == 1 )
    {
        setDescription( m_progressMap.values().at( 0 )->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
    }
    else
    {
        setDescription( i18n( "Multiple background tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background tasks" ) );
    }

    if ( m_progressMap.count() == 0 )
    {
        progressBar()->setValue( 0 );
        hideDetails();
        emit( allDone() );
        m_progressDetailsWidget->hide();
        return;
    }

    progressBar()->setValue( calcCompoundPercentage() );

    handleDetailsButton();
}

int CompoundProgressBar::calcCompoundPercentage()
{
    int count = m_progressMap.count();
    int total = 0;

    foreach( ProgressBar * currentBar, m_progressMap )
        total += currentBar->percentage();

    return count == 0 ? 0 : total / count;
}

void CompoundProgressBar::cancelAll()
{
    DEBUG_BLOCK

    foreach( ProgressBar * currentBar, m_progressMap )
        currentBar->cancel();
}

void CompoundProgressBar::showDetails()
{
    DEBUG_BLOCK
    m_progressDetailsWidget->raise();

    //Hack to make sure it has the right heigh first time it is shown...
    m_progressDetailsWidget->setFixedHeight( m_progressMap.values().at( 0 )->height() * m_progressMap.count() + 8 );
    m_progressDetailsWidget->reposition();
    m_progressDetailsWidget->show();

    m_showDetailsButton->setIcon( KIcon( "arrow-down-double-amarok" ) );
}

void CompoundProgressBar::hideDetails()
{
    m_progressDetailsWidget->hide();
    m_showDetailsButton->setIcon( KIcon( "arrow-up-double-amarok" ) );
    handleDetailsButton();
}

void CompoundProgressBar::toggleDetails()
{
    if ( m_progressDetailsWidget->isVisible() )
        hideDetails();
    else
        showDetails();
}

void CompoundProgressBar::handleDetailsButton()
{
    if ( m_progressMap.count() > 1 )
        m_showDetailsButton->show();
    else if ( !m_progressDetailsWidget->isVisible() )
        m_showDetailsButton->hide();
}

