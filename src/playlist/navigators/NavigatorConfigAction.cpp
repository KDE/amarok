/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "NavigatorConfigAction.h"

#include <KMenu>
#include <KLocale>
#include <KStandardDirs>

NavigatorConfigAction::NavigatorConfigAction( QWidget * parent )
    : KAction( parent )
{

    setIcon( KIcon( "media-playlist-repeat-amarok" ) );
    KMenu * navigatorMenu = new KMenu( parent );
    setMenu( navigatorMenu );
    setText( i18n( "Track Progression" ) );

    QActionGroup * navigatorActions = new QActionGroup( navigatorMenu );
    navigatorActions->setExclusive( true );

    QAction * action;
    action = navigatorActions->addAction( i18n( "Standard" ) );
    action->setCheckable( true );
    //action->setIcon( true );

    action = new QAction( parent );
    action->setSeparator( true );
    navigatorActions->addAction( action );
    
    action = navigatorActions->addAction( i18n( "Repeat Track" ) );
    action->setCheckable( true );
        
    action = navigatorActions->addAction( i18n( "Repeat Album" ) );
    action->setCheckable( true );
        
    action = navigatorActions->addAction( i18n( "Repeat Playlist" ) );
    action->setCheckable( true );
        
    action = new QAction( parent );
    action->setSeparator( true );
    navigatorActions->addAction( action );
    
    action = navigatorActions->addAction( i18n( "Random Tracks" ) );
    action->setCheckable( true );
        
    action = navigatorActions->addAction( i18n( "Random Albums" ) );
    action->setCheckable( true );

    navigatorMenu->addActions( navigatorActions->actions() );
        
    QMenu * favorMenu = navigatorMenu->addMenu( i18n( "Favor" ) );
    QActionGroup * favorActions = new QActionGroup( favorMenu );

    action = favorActions->addAction( i18n( "None" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Higher Scores" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Higher Ratings" ) );
    action->setCheckable( true );
    
    action = favorActions->addAction( i18n( "Not Recently Played" ) );
    action->setCheckable( true );

    favorMenu->addActions( favorActions->actions() );
    
}

NavigatorConfigAction::~NavigatorConfigAction()
{
}

void NavigatorConfigAction::setActiveNavigator( QAction *navigatorAction )
{

}

#include "NavigatorConfigAction.moc"