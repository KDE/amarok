/***************************************************************************
 * copyright     : (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "SearchWidget.h"
#include "EditFilterDialog.h"

#include <QVBoxLayout>

#include <KLineEdit>
#include <KLocale>
#include <KHBox>
#include <KPushButton>


SearchWidget::SearchWidget( QWidget *parent, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterButton( 0 )
{
    init( parent, advanced );
}

SearchWidget::SearchWidget( QWidget *parent, QWidget *caller, bool advanced )
    : QWidget( parent )
    , m_sw( 0 )
    , m_filterButton( 0 )
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
}

///Private
void
SearchWidget::init( QWidget *parent, bool advanced )
{
    Q_UNUSED( parent )
    setContentsMargins( 0, 0, 0, 0 );
    KHBox *searchBox = new KHBox( this );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );

    m_sw = new KLineEdit( searchBox );
    m_sw->setClickMessage( i18n( "Enter search terms here" ) );
    m_sw->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_sw->setClearButtonShown( true );
    m_sw->setFrame( true );
    m_sw->setToolTip( i18n( "Enter space-separated terms to search in the playlist." ) );
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( searchBox );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );

    if ( advanced ) {
        m_filterButton = new KPushButton( i18n( "Advanced" ), searchBox );
        //m_filterButton->setFlat( true ); //TODO: maybe?
        m_filterButton->setObjectName( "filter" );
        m_filterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
        m_filterButton->setToolTip( i18n( "Click to edit playlist filter" ) );

        connect ( m_filterButton, SIGNAL( clicked() ), this, SLOT( slotShowFilterEditor() ) );
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

#include "SearchWidget.moc"

