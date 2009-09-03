/****************************************************************************************
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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

#include "SearchWidget.h"
#include "EditFilterDialog.h"

#include <QVBoxLayout>

#include <KIcon>
#include <KLocale>
#include <KHBox>
#include <KPushButton>


SearchWidget::SearchWidget( QWidget *parent, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterAction( 0 )
{
    init( parent, advanced );
}

SearchWidget::SearchWidget( QWidget *parent, QWidget *caller, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterAction( 0 )
{
    init( parent, advanced );
    setup( caller );
}

void
SearchWidget::setup( QObject* caller )
{
    connect( m_sw, SIGNAL( textChanged( const QString & ) ), caller,
             SLOT( slotSetFilterTimeout() ) );
    connect( this, SIGNAL( filterNow() ), caller,
             SLOT( slotFilterNow() ) );
    connect( m_sw, SIGNAL( returnPressed() ), caller, SLOT( slotFilterNow() ) );
    connect( m_sw, SIGNAL( downPressed() ), caller, SLOT( setFocus() ) );
    connect( caller, SIGNAL( leavingTree() ), m_sw, SLOT( setFocus() ) );
}

///Private
void
SearchWidget::init( QWidget *parent, bool advanced )
{
    Q_UNUSED( parent )
    setContentsMargins( 0, 0, 0, 0 );
    KHBox *searchBox = new KHBox( this );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

    m_sw = new Amarok::LineEdit( searchBox );
    m_sw->setClickMessage( i18n( "Enter search terms here" ) );
    m_sw->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_sw->setClearButtonShown( true );
    m_sw->setFrame( true );
    m_sw->setToolTip( i18n( "Enter space-separated terms to search." ) );
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( searchBox );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );

    m_toolBar = new QToolBar( searchBox );
    m_toolBar->setFixedHeight( m_sw->sizeHint().height() );

    if ( advanced ) {
        m_filterAction = new QAction( KIcon( "document-properties" ), i18n( "Edit filter" ), this );
        m_filterAction->setObjectName( "filter" );
        m_toolBar->addAction( m_filterAction );

        connect ( m_filterAction, SIGNAL( triggered() ), this, SLOT( slotShowFilterEditor() ) );
    }
}

void
SearchWidget::setSearchString( const QString &searchString )
{
    m_sw->setText( searchString );
    emit filterNow();
}

void
SearchWidget::slotShowFilterEditor()
{
    EditFilterDialog *fd = new EditFilterDialog( this, m_sw->text() );

    connect( fd, SIGNAL( filterChanged( const QString & ) ), m_sw,  SLOT( setText( const QString & ) ) );

    fd->exec();
}

QToolBar * SearchWidget::toolBar()
{
    return m_toolBar;
}

void SearchWidget::showAdvancedButton(bool show)
{
    if ( show ) {
        if ( m_filterAction != 0 ) {
            m_filterAction = new QAction( KIcon( "document-properties" ), i18n( "Edit filter" ), this );
            m_filterAction->setObjectName( "filter" );
            m_toolBar->addAction( m_filterAction );
            connect ( m_filterAction, SIGNAL( triggered() ), this, SLOT( slotShowFilterEditor() ) );
        }
    }
    else
    {
        delete m_filterAction;
        m_filterAction = 0;
    }
}

void SearchWidget::setClickMessage( const QString &message )
{
    m_sw->setClickMessage( message );
}


#include "SearchWidget.moc"



