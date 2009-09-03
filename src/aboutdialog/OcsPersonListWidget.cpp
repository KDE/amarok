/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "OcsPersonListWidget.h"

#include "Debug.h"

#include <QScrollArea>


OcsPersonListWidget::OcsPersonListWidget( const QList< KAboutPerson > &persons, const OcsData::OcsPersonList *ocsPersons, OcsPersonItem::PersonStatus status, QWidget *parent )
    : QWidget( parent )
    , m_status( status )
    , m_fetchCount( 0 )
{
    //Set up the layouts...
    QHBoxLayout *scrollLayout = new QHBoxLayout( this );
    scrollLayout->setMargin( 1 );
    setLayout( scrollLayout );
    QScrollArea *personsScrollArea = new QScrollArea( this );
    scrollLayout->addWidget( personsScrollArea );
    personsScrollArea->setFrameStyle( QFrame::NoFrame );
    m_personsArea = new QWidget( personsScrollArea );
    m_areaLayout = new QVBoxLayout( m_personsArea );
    m_areaLayout->setMargin( 0 );
    m_personsArea->setLayout( m_areaLayout );
    m_personsArea->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );


    personsScrollArea->setWidgetResizable( true );
    personsScrollArea->setWidget( m_personsArea );
    m_personsArea->show();

    //Populate the scroll area...
    for( int i = 0; i < persons.count(); ++i )  //TODO: really ugly and inefficient, fix this
    {
        OcsPersonItem *item = new OcsPersonItem( persons.at( i ), ocsPersons->at( i ).first, status, m_personsArea );
        m_areaLayout->addWidget( item );
        connect( item, SIGNAL( ocsFetchStarted() ), this, SLOT( onOcsFetchStarted() ) );
        connect( item, SIGNAL( ocsFetchResult( int ) ), this, SLOT( onOcsDataFetched( int ) ) );
    }
}

void
OcsPersonListWidget::switchToOcs()
{
    for( int i = 0; i < m_areaLayout->count(); ++i )
    {
        OcsPersonItem *item = qobject_cast< OcsPersonItem * >( m_areaLayout->itemAt( i )->widget() );
        item->switchToOcs();
    }
}

void
OcsPersonListWidget::onOcsFetchStarted()        //SLOT
{
    m_fetchCount++;
}

void
OcsPersonListWidget::onOcsDataFetched( int err )    //SLOT
{
    m_fetchCount--;
    debug()<<m_status<<"Fetch count is"<< m_fetchCount;
    if( err != 0 )
        warning() << "OCS data download failed with error"<<err;
    if( m_fetchCount == 0 )
    {
        debug()<<m_status<<"FETCH COMPLETE";
        emit switchedToOcs();
    }
}
