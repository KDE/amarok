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

#include "PlaylistGroupingAction.h"

#include "playlist/PlaylistDefines.h"
#include "playlist/PlaylistModelStack.h"

namespace Playlist
{

GroupingAction::GroupingAction( QWidget *parent )
    : KAction( parent )
{
    setIcon( KIcon( "object-group" ) );
    m_groupingMenu = new KMenu( parent );
    setMenu( m_groupingMenu );
    setText( i18n( "Group &by" ) );

    m_groupingActions = new QActionGroup( m_groupingMenu );
    m_groupingActions->setExclusive( true );

    QStringList groupingList( groupableCategories );
    foreach( QString it, groupingList )
    {
        QString prettyCategoryName = columnNames.at( internalColumnNames.indexOf( it ) );
        QString iconName = iconNames.at( internalColumnNames.indexOf( it ) );
        QAction *action = m_groupingActions->addAction( KIcon( iconName ), prettyCategoryName );
        action->setCheckable( true );
        action->setData( it );
    }
    m_groupingMenu->addActions( m_groupingActions->actions() );
    int index = groupableCategories.indexOf( Playlist::ModelStack::instance()->top()->groupingCategory() );
    if( index > -1 )
        m_groupingActions->actions()[ index ]->setChecked( true );
    connect( m_groupingActions, SIGNAL( triggered( QAction* ) ), this, SLOT( setGrouping( QAction* ) ) );
}

GroupingAction::~GroupingAction()
{}

void
GroupingAction::setGrouping( QAction *groupingAction )
{
    QString groupingCategory = groupingAction->data().toString();
    Playlist::ModelStack::instance()->top()->setGroupingCategory( groupingCategory );
}

}   //namespace Playlist
