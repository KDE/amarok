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

SearchWidget::SearchWidget( QWidget *parent, QWidget *caller )
    : QWidget( parent ),
      m_sw( 0 )
{
    m_caller = caller;
    KHBox *searchBox = new KHBox( parent );
    searchBox->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

    m_sw = new KLineEdit( searchBox );
    m_sw->setClickMessage( i18n( "Enter search terms here" ) );
    m_sw->setClearButtonShown( true );
    m_sw->setFrame( QFrame::Sunken );
    m_sw->setToolTip( i18n(
                                "Enter space-separated terms to search in the playlist." ) );

    KPushButton *filterButton = new KPushButton( "...", searchBox );
    filterButton->setFlat( true ); //TODO: maybe?
    filterButton->setObjectName( "filter" );
    filterButton->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
    filterButton->setToolTip( i18n( "Click to edit playlist filter" ) );

    connect( filterButton, SIGNAL( clicked() ), m_caller,
             SLOT(slotEditFilter() ) );
    connect( m_sw, SIGNAL( textChanged( const QString & ) ), m_caller,
             SLOT( slotSetFilterTimeout() ) );
}

#include "searchwidget.moc"