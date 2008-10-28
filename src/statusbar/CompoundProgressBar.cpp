/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "CompoundProgressBar.h"

#include "Debug.h"
#include "MainWindow.h"

#include <KIcon>
#include <KLocale>
#include <KVBox>

#include <QDialog>
#include <QVBoxLayout>

CompoundProgressBar::CompoundProgressBar( QWidget * parent )
        : ProgressBarNG( parent )
{
    m_showDetailsButton = new QToolButton( extrabuttonSpace() );
    m_showDetailsButton->setIcon( KIcon( "arrow-up-double-amarok" ) );

    m_progressDetailsWidget = new PopupWidget( this );
    m_progressDetailsWidget->hide();
    //connect( m_progressDetailsWidget, SIGNAL( finished ( int ) ), this, SLOT( detailsWindowClosed() ) );
    
    connect( m_showDetailsButton, SIGNAL( clicked() ), this, SLOT( toggleDetails() ) );
}

CompoundProgressBar::~CompoundProgressBar()
{}

void CompoundProgressBar::addProgressBar( ProgressBarNG * childBar, QObject *owner )
{
    DEBUG_BLOCK

    m_progressMap.insert( owner, childBar );
    m_progressDetailsWidget->layout()->addWidget( childBar );
    if ( m_progressDetailsWidget->width() < childBar->width() )
    {
        m_progressDetailsWidget->setMinimumWidth( childBar->width() );
    }

    debug() << "setting fixed height: " << ( childBar->height() + 4 ) << " * " << m_progressMap.count() << " = " << childBar->height() * m_progressMap.count() + 8;

    m_progressDetailsWidget->setMinimumHeight( childBar->height() * m_progressMap.count()  + 8 );

    m_progressDetailsWidget->reposition();

    debug() << "we now have " << m_progressMap.count() << " progress ops running";

    connect( childBar, SIGNAL( percentageChanged( int ) ), this, SLOT( childPercentageChanged() ) );
    connect( childBar, SIGNAL( cancelled( ProgressBarNG * ) ), this, SLOT( childBarCancelled( ProgressBarNG * ) ) );
    connect( childBar, SIGNAL( complete( ProgressBarNG * ) ), this, SLOT( childBarComplete( ProgressBarNG * ) ) );

    if ( m_progressMap.count() == 1 )
    {
        setDescription( childBar->descriptionLabel()->text() );
        cancelButton()->setToolTip( i18n( "Abort" ) );
    }
    else
    {
        setDescription( i18n( "Multiple background-tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background operations" ) );
    }

    connect( cancelButton(), SIGNAL( clicked() ), this, SLOT( cancelAll() ) );
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
    progresBar()->setValue( calcCompoundPercentage() );
}

void CompoundProgressBar::childBarCancelled( ProgressBarNG * childBar )
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
        setDescription( i18n( "Multiple background-tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background operations" ) );
    }

    if ( m_progressMap.count() == 0 )
    {
        m_progressDetailsWidget->setMinimumWidth( 0 );
        cancelButton()->setEnabled( false );
        hideDetails();
        emit( allDone() );
        return;
    }

    progresBar()->setValue( calcCompoundPercentage() );

    handleDetailsButton();
}

void CompoundProgressBar::childBarComplete( ProgressBarNG * childBar )
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
        setDescription( i18n( "Multiple background-tasks running" ) );
        cancelButton()->setToolTip( i18n( "Abort all background operations" ) );
    }

    if ( m_progressMap.count() == 0 )
    {
        hideDetails();
        emit( allDone() );
        m_progressDetailsWidget->hide();
        return;
    }

    progresBar()->setValue( calcCompoundPercentage() );

    handleDetailsButton();
}

int CompoundProgressBar::calcCompoundPercentage()
{

    int count = m_progressMap.count();
    int total = 0;

    foreach( ProgressBarNG * currentBar, m_progressMap.values() )
    {
        total += currentBar->percentage();
    }

    return total / count;

}

void CompoundProgressBar::cancelAll()
{
    DEBUG_BLOCK

    foreach( ProgressBarNG * currentBar, m_progressMap.values() )
        currentBar->cancel();
}

void CompoundProgressBar::showDetails()
{

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

