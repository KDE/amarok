/****************************************************************************************
 * Copyright (c) 2009-2010 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "PlaylistBrowserCategory.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "playlist/PlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"
#include "PlaylistsInGroupsProxy.h"
#include "PlaylistsByProviderProxy.h"
#include "PlaylistTreeItemDelegate.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "PlaylistBrowserView.h"

#include <KAction>
#include <KActionMenu>
#include <KButtonGroup>
#include <KIcon>
#include <KLineEdit>

#include <QButtonGroup>
#include <QCheckBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <KStandardDirs>
#include <QToolBar>
#include <QVBoxLayout>

#include <typeinfo>

#include "PlaylistBrowserModel.h"

using namespace PlaylistBrowserNS;

QString PlaylistBrowserCategory::s_mergeViewKey( "Merged View" );

PlaylistBrowserCategory::PlaylistBrowserCategory( int playlistCategory,
                                                  QString categoryName,
                                                  QString configGroup,
                                                  PlaylistBrowserModel *model,
                                                  QWidget *parent ) :
    BrowserCategory( categoryName, parent ),
    m_configGroup( configGroup ),
    m_playlistCategory( playlistCategory )
{
    setContentsMargins( 0, 0, 0, 0 );
    KToolBar *toolBar = new KToolBar( this, false, false );
    toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    m_byProviderProxy = new PlaylistsByProviderProxy( model, PlaylistBrowserModel::ProviderColumn );
    m_byFolderProxy = new PlaylistsInGroupsProxy( model );

    m_filterProxy = new QSortFilterProxyModel( this );
    m_filterProxy->setDynamicSortFilter( true );
    m_filterProxy->setFilterKeyColumn( PlaylistBrowserModel::ProviderColumn );

    m_playlistView = new PlaylistBrowserView( m_filterProxy, this );
    m_defaultItemDelegate = m_playlistView->itemDelegate();
    m_byProviderDelegate = new PlaylistTreeItemDelegate( m_playlistView );

    m_addFolderAction = new KAction( KIcon( "folder-new" ), i18n( "Add Folder" ), this  );
    toolBar->addAction( m_addFolderAction );
    connect( m_addFolderAction, SIGNAL( triggered( bool ) ), SLOT( createNewFolder() ) );

    //a QWidget with minimumExpanding makes the next button right aligned.
    QWidget *spacerWidget = new QWidget( this );
    spacerWidget->setSizePolicy( QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding );
    toolBar->addWidget( spacerWidget );

    m_providerMenu = new KActionMenu( KIcon( "checkbox" ), i18n( "Visible Sources"), this );
    m_providerMenu->setDelayed( false );
    toolBar->addAction( m_providerMenu );

    KAction *toggleAction = new KAction( KIcon( "view-list-tree" ), QString(), toolBar );
    toggleAction->setToolTip( i18n( "Merged View" ) );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( Amarok::config( m_configGroup ).readEntry( s_mergeViewKey, false ) );
    toolBar->addAction( toggleAction );
    connect( toggleAction, SIGNAL( triggered( bool ) ), SLOT( toggleView( bool ) ) );

    toggleView( toggleAction->isChecked() );

    m_playlistView->setFrameShape( QFrame::NoFrame );
    m_playlistView->setContentsMargins( 0, 0, 0, 0 );
    m_playlistView->setAlternatingRowColors( true );
    m_playlistView->header()->hide();
    //hide all columns except the first.
    for( int i = 1; i < m_playlistView->model()->columnCount(); i++ )
      m_playlistView->hideColumn( i );

    m_playlistView->setDragEnabled( true );
    m_playlistView->setAcceptDrops( true );
    m_playlistView->setDropIndicatorShown( true );

    foreach( const Playlists::PlaylistProvider *provider,
             The::playlistManager()->providersForCategory( m_playlistCategory ) )
    {
        createProviderButton( provider );
    }

    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ),
             SLOT( newPalette( const QPalette & ) ) );
}

PlaylistBrowserCategory::~PlaylistBrowserCategory()
{
}

void
PlaylistBrowserCategory::toggleView( bool merged )
{
    if( merged )
    {
        m_filterProxy->setSourceModel( m_byFolderProxy );
        m_playlistView->setItemDelegate( m_defaultItemDelegate );
        m_playlistView->setRootIsDecorated( true );
    }
    else
    {
        m_filterProxy->setSourceModel( m_byProviderProxy );
        m_playlistView->setItemDelegate( m_byProviderDelegate );
        m_playlistView->setRootIsDecorated( false );
    }

    //folders don't make sense in per-provider view
    m_addFolderAction->setEnabled( merged );
    //TODO: set a tooltip saying why it's disabled mention labels

    Amarok::config( m_configGroup ).writeEntry( s_mergeViewKey, merged );
}

void
PlaylistBrowserCategory::slotProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    Q_UNUSED( category )

    if( !m_providerActions.keys().contains( provider ) )
        createProviderButton( provider );
}

void
PlaylistBrowserCategory::slotProviderRemoved( Playlists::PlaylistProvider *provider, int category )
{
    Q_UNUSED( category )

    if( m_providerActions.keys().contains( provider ) )
    {
        QAction *providerToggle = m_providerActions.take( provider );
        m_providerMenu->removeAction( providerToggle );
    }
}


void
PlaylistBrowserCategory::createProviderButton( const Playlists::PlaylistProvider *provider )
{
    QAction *providerToggle = new QAction( provider->icon(), provider->prettyName(), this );
    providerToggle->setCheckable( true );
    providerToggle->setChecked( true );
    providerToggle->setData( QVariant::fromValue( provider ) );
    connect( providerToggle, SIGNAL(toggled(bool)), SLOT(slotToggleProviderButton(bool)) );
    m_providerMenu->addAction( providerToggle );
    m_providerActions.insert( provider, providerToggle );
}

void
PlaylistBrowserCategory::slotToggleProviderButton( bool enabled )
{
    Q_UNUSED( enabled )
    DEBUG_BLOCK

    QAction * const action = qobject_cast<QAction *>( QObject::sender() );
    const Playlists::PlaylistProvider *provider = action->data().value<const Playlists::PlaylistProvider *>();
    if( !m_providerActions.keys().contains( provider ) )
        return;

    QString filter;
    QActionList checkedActions;
    foreach( const Playlists::PlaylistProvider *p, m_providerActions.keys() )
    {
        QAction *action = m_providerActions.value( p );
        if( action->isChecked() )
        {
            QString escapedName = QRegExp::escape( p->prettyName() ).replace( " ", "\\ " );
            filter += QString( filter.isEmpty() ? "%1" : "|%1" ).arg( escapedName );
            checkedActions << action;
            action->setEnabled( true );
        }
    }
    //if all are enabled the filter can be completely disabled.
    if( checkedActions.count() == m_providerActions.count() )
        filter = QString();

    m_filterProxy->setFilterRegExp( filter );

    //don't allow the last visible provider to be hidden
    if( checkedActions.count() == 1 )
        checkedActions.first()->setEnabled( false );
}

void
PlaylistBrowserCategory::createNewFolder()
{
    // TODO: Determine how many "New Folder" entries there are and set n accordingly.
    int n = 1;
    QString name;
    if ( n == 1 )
        name = i18nc( "default name for new folder", "New Folder" );
    else
        name = i18nc( "default name for new folder", "New Folder (%1)", n );
    QModelIndex idx = m_byFolderProxy->createNewGroup( name );
    m_playlistView->edit( m_filterProxy->mapFromSource( idx ) );
}

void
PlaylistBrowserCategory::newPalette( const QPalette &palette )
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}
