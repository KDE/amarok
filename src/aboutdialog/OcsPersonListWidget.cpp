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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "OcsPersonListWidget.h"

#include "Debug.h"

#include <QScrollArea>


OcsPersonListWidget::OcsPersonListWidget( QWidget *parent )
    : QWidget( parent )
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
}

void
OcsPersonListWidget::addPerson( const KAboutPerson &person, const Attica::Person &ocsPerson )
{
    OcsAuthorItem *item = new OcsAuthorItem( person, ocsPerson, m_personsArea );
    addPersonPrivate( item );
}
void
OcsPersonListWidget::addPerson( const KAboutPerson &person )
{
    OcsAuthorItem *item = new OcsAuthorItem( person, m_personsArea );
    addPersonPrivate( item );
}

void
OcsPersonListWidget::addPersonPrivate( OcsAuthorItem *item )
{
    if( m_areaLayout->count() == 0 )
        m_areaLayout->addWidget( item );
    else
    {
        QString name = item->name();
        for( int i = m_areaLayout->count() - 1; i >= 0; --i )
        {
            QString currentName = qobject_cast< OcsAuthorItem * >( m_areaLayout->itemAt( i )->widget() )->name();
            if( name > currentName )
            {
                debug()<<"Inserting"<< currentName;
                m_areaLayout->insertWidget( i + 1, item );
                break;
            }
            if( i == 0 )
                m_areaLayout->insertWidget( 0, item );
        }
    }
    emit personAdded( m_areaLayout->count() );
}
