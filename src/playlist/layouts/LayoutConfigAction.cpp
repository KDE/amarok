/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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
 
#include "LayoutConfigAction.h"

#include "core/support/Debug.h"
#include "LayoutManager.h"
#include "PlaylistLayoutEditDialog.h"
#include "MainWindow.h"

#include <QActionGroup>
#include <QPixmap>
#include <QStandardPaths>

namespace Playlist
{

LayoutConfigAction::LayoutConfigAction( QWidget * parent )
    : QAction( parent )
    , m_layoutDialog( nullptr )
{
    QIcon actionIcon( QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/playlist-layouts-22.png")) ) );    //TEMPORARY ICON
    setIcon( actionIcon );
    m_layoutMenu = new QMenu( parent );
    setMenu( m_layoutMenu );
    setText( i18n( "Playlist Layouts" ) );
    m_configAction = new QAction( m_layoutMenu );
    
    m_layoutMenu->addAction( m_configAction );
    m_layoutMenu->addSeparator();
    m_layoutActions = new QActionGroup( m_layoutMenu );
    m_layoutActions->setExclusive( true );

    QStringList layoutsList( LayoutManager::instance()->layouts() );
    for( const QString &iterator : layoutsList )
    {
        m_layoutActions->addAction( iterator )->setCheckable( true );
    }
    m_layoutMenu->addActions( m_layoutActions->actions() );
    int index = LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() );
    if( index > -1 )    //needed to avoid crash when created a layout which is moved by the LayoutManager when sorting alphabetically.
                        //this should be fixed by itself when layouts ordering will be supported in the LayoutManager
    m_layoutActions->actions()[ index ]->setChecked( true );

    connect( m_layoutActions,&QActionGroup::triggered, this, &LayoutConfigAction::setActiveLayout );

    connect( LayoutManager::instance(), &LayoutManager::layoutListChanged, this, &LayoutConfigAction::layoutListChanged );
    connect( LayoutManager::instance(), &LayoutManager::activeLayoutChanged, this, &LayoutConfigAction::onActiveLayoutChanged );

    const QIcon configIcon( QStringLiteral("configure") );
    m_configAction->setIcon( configIcon );
    m_configAction->setText( i18n( "Configure Playlist Layouts..." ) );

    connect( m_configAction, &QAction::triggered, this, &LayoutConfigAction::configureLayouts );
}


LayoutConfigAction::~LayoutConfigAction()
{}

void LayoutConfigAction::setActiveLayout( QAction *layoutAction )
{
    QString layoutName( layoutAction->text() );
    layoutName = layoutName.remove( QLatin1Char( '&' ) );        //need to remove the & from the string, used for the shortcut key underscore
    LayoutManager::instance()->setActiveLayout( layoutName );
}

void LayoutConfigAction::configureLayouts()
{
    if( m_layoutDialog == nullptr )
        m_layoutDialog = new PlaylistLayoutEditDialog( The::mainWindow() );

    m_layoutDialog->setModal( false );
    connect( m_layoutDialog, &Playlist::PlaylistLayoutEditDialog::accepted, this, &LayoutConfigAction::layoutListChanged );

    m_layoutDialog->show();
}

void Playlist::LayoutConfigAction::layoutListChanged()
{
    m_layoutMenu->removeAction( m_configAction );
    m_layoutMenu->clear();
    m_layoutMenu->addAction( m_configAction );
    m_layoutMenu->addSeparator();
    
    for( QAction * action : m_layoutActions->actions() )
        delete action;
    
    QStringList layoutsList( LayoutManager::instance()->layouts() );
    for( const QString &iterator : layoutsList )
        m_layoutActions->addAction( iterator )->setCheckable( true );
    
    m_layoutMenu->addActions( m_layoutActions->actions() );
    
    int index = LayoutManager::instance()->layouts().indexOf( LayoutManager::instance()->activeLayoutName() );
    if( index > -1 )    //needed to avoid crash when created a layout which is moved by the LayoutManager when sorting alphabetically.
                        //this should be fixed by itself when layouts ordering will be supported in the LayoutManager
        m_layoutActions->actions()[ index ]->setChecked( true );
}

void LayoutConfigAction::onActiveLayoutChanged()
{
    QString layoutName( LayoutManager::instance()->activeLayoutName() );
    layoutName = layoutName.remove( QLatin1Char( '&' ) );        //need to remove the & from the string, used for the shortcut key underscore
    if( layoutName != QStringLiteral( "%%PREVIEW%%" ) )           //if it's not just a preview
    {
        int index = LayoutManager::instance()->layouts().indexOf( layoutName );
        if( index != -1 && m_layoutActions->actions()[ index ] != m_layoutActions->checkedAction() )
            m_layoutActions->actions()[ index ]->setChecked( true );
    }
}

}

