/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "SourceSelectionPopup.h"

#include <KIcon>
#include <KLocale>

#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

namespace Playlist {

    SourceSelectionPopup::SourceSelectionPopup( QWidget * parent,  Meta::MultiSourceCapability * msc )
    : QDialog( parent )
    , m_msc( msc )
{

    QLabel * label = new QLabel( i18n( "The following sources are available for this track:" ) );
    label->setWordWrap( true );
    
    m_listWidget = new QListWidget();

    QPushButton * okButton = new QPushButton( i18n( "Ok" ) );
    connect( okButton, SIGNAL( clicked() ), SLOT( accept() ) );

    connect( m_listWidget, SIGNAL( itemDoubleClicked( QListWidgetItem * ) ), this, SLOT( sourceSelected( QListWidgetItem * ) ) );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget( label );
    layout->addWidget( m_listWidget );
    layout->addWidget( okButton );
    setLayout( layout );

    int i = 0;
    foreach( QString source, m_msc->sources() )
    {
        if ( i == m_msc->current() )
            new QListWidgetItem( KIcon( "arrow-right" ), source, m_listWidget ) ;
        else
            new QListWidgetItem( source, m_listWidget );

        i++;
    }
}

SourceSelectionPopup::~SourceSelectionPopup()
{
    delete m_msc;
}

void SourceSelectionPopup::sourceSelected( QListWidgetItem * item )
{

    //get row of item:

    int currentSource = m_listWidget->row( item );

    m_msc->setSource( currentSource );

    m_listWidget->clear();
    
    int i = 0;
    foreach( QString source, m_msc->sources() )
    {
        if ( i == m_msc->current() )
            new QListWidgetItem( KIcon( "arrow-right" ), source, m_listWidget ) ;
        else
            new QListWidgetItem( source, m_listWidget );

        i++;
    }

}


}

#include "SourceSelectionPopup.moc"


