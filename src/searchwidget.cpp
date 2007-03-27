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
#include "searchwidget.h"

#include <klineedit.h>
#include <klocale.h>
#include <khbox.h>
#include <kpushbutton.h>

SearchWidget::SearchWidget( QWidget *parent )
    : QWidget( parent ),
      m_sw( 0 ),
      m_filterButton( 0 )
{
    init( parent );
}

SearchWidget::SearchWidget( QWidget *parent, QWidget* caller )
    : QWidget( parent ),
      m_sw( 0 ),
      m_filterButton( 0 )
{
    init( parent );
    setup( caller );
}

void
SearchWidget::setup( QObject* caller )
{
    connect( m_filterButton, SIGNAL( clicked() ), caller,
             SLOT(slotEditFilter() ) );
    connect( m_sw, SIGNAL( textChanged( const QString & ) ), caller,
             SLOT( slotSetFilterTimeout() ) );
}

///Private
void
SearchWidget::init( QWidget *parent )
{
    KHBox *searchBox = new KHBox( this );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

    m_sw = new KLineEdit( searchBox );
    m_sw->setClickMessage( i18n( "Enter search terms here" ) );
    m_sw->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_sw->setClearButtonShown( true );
    m_sw->setFrame( QFrame::Sunken );
    m_sw->setToolTip( i18n(
                                "Enter space-separated terms to search in the playlist." ) );

    m_filterButton = new KPushButton( "...", searchBox );
    m_filterButton->setFlat( true ); //TODO: maybe?
    m_filterButton->setObjectName( "filter" );
    m_filterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    m_filterButton->setToolTip( i18n( "Click to edit playlist filter" ) );
}

#include "searchwidget.moc"