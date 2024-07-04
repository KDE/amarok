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

#define DEBUG_PREFIX "PlaylistBrowserCategory"

#include "PlaylistBrowserCategory.h"

#include "amarokconfig.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "PlaylistBrowserModel.h"
#include "playlist/PlaylistModel.h"
#include "playlistmanager/PlaylistManager.h"
#include "PlaylistsInFoldersProxy.h"
#include "PlaylistsByProviderProxy.h"
#include "PlaylistBrowserFilterProxy.h"
#include "PlaylistBrowserView.h"
#include "widgets/PrettyTreeDelegate.h"

#include <KActionMenu>
#include <KToolBar>

#include <QHeaderView>
#include <QIcon>
#include <QRegularExpression>
#include <QStandardPaths>

#include <typeinfo>

using namespace PlaylistBrowserNS;

QString PlaylistBrowserCategory::s_mergeViewKey( QStringLiteral("Merged View") );

PlaylistBrowserCategory::PlaylistBrowserCategory( int playlistCategory,
                                                  const QString &categoryName,
                                                  const QString &configGroup,
                                                  PlaylistBrowserModel *model,
                                                  QWidget *parent ) :
    BrowserCategory( categoryName, parent ),
    m_configGroup( configGroup ),
    m_playlistCategory( playlistCategory )
{
    setContentsMargins( 0, 0, 0, 0 );
    setImagePath( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/hover_info_podcasts.png") ) );

    // set background
    if( AmarokConfig::showBrowserBackgroundImage() )
        setBackgroundImage( imagePath() );

    m_toolBar = new KToolBar( this, false, false );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    //a QWidget with minimumExpanding makes the next button right aligned.
    QWidget *spacerWidget = new QWidget( this );
    spacerWidget->setSizePolicy( QSizePolicy::MinimumExpanding,
                                 QSizePolicy::MinimumExpanding );
    // add a separator so subclasses can add their actions before it
    m_separator = m_toolBar->addWidget( spacerWidget );

    m_toolBar->addSeparator();

    m_addFolderAction = new QAction( QIcon::fromTheme( QStringLiteral("folder-new") ), i18n( "Add Folder" ), this  );
    m_addFolderAction->setPriority( QAction::LowPriority );
    m_toolBar->addAction( m_addFolderAction );
    connect( m_addFolderAction, &QAction::triggered, this, &PlaylistBrowserCategory::createNewFolder );

    m_providerMenu = new KActionMenu( QIcon::fromTheme( QStringLiteral("checkbox") ), i18n( "Visible Sources"), this );
    m_providerMenu->setPopupMode( QToolButton::InstantPopup );
    m_providerMenu->setPriority( QAction::HighPriority );
    m_toolBar->addAction( m_providerMenu );

    QAction *toggleAction = new QAction( QIcon::fromTheme( QStringLiteral("view-list-tree") ), i18n( "Merged View" ),
                                         m_toolBar );
    toggleAction->setCheckable( true );
    toggleAction->setChecked( Amarok::config( m_configGroup ).readEntry( s_mergeViewKey, false ) );
    toggleAction->setPriority( QAction::LowPriority );
    m_toolBar->addAction( toggleAction );
    connect( toggleAction, &QAction::triggered, this, &PlaylistBrowserCategory::toggleView );

    m_toolBar->addSeparator();

    m_byProviderProxy = new PlaylistsByProviderProxy( m_playlistCategory, this );
    m_byProviderProxy->setSourceModel( model );
    m_byProviderProxy->setGroupedColumn( PlaylistBrowserModel::ProviderColumn );
    m_byFolderProxy = new PlaylistsInFoldersProxy( model );

    m_filterProxy = new PlaylistBrowserFilterProxy( this );
    //no need to setModel on filterProxy since it will be done in toggleView anyway.
    m_filterProxy->setDynamicSortFilter( true );
    m_filterProxy->setFilterKeyColumn( PlaylistBrowserModel::ProviderColumn );

    m_playlistView = new PlaylistBrowserView( m_filterProxy, this );
    m_defaultItemDelegate = m_playlistView->itemDelegate();
    m_byProviderDelegate = new PrettyTreeDelegate( m_playlistView );

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

    for( const Playlists::PlaylistProvider *provider :
             The::playlistManager()->providersForCategory( m_playlistCategory ) )
    {
        createProviderButton( provider );
    }

    connect( The::playlistManager(), &PlaylistManager::providerAdded,
             this, &PlaylistBrowserCategory::slotProviderAdded );
    connect( The::playlistManager(), &PlaylistManager::providerRemoved,
             this, &PlaylistBrowserCategory::slotProviderRemoved );

    connect( The::paletteHandler(), &PaletteHandler::newPalette,
             this, &PlaylistBrowserCategory::newPalette );
}

PlaylistBrowserCategory::~PlaylistBrowserCategory()
{
}

QString
PlaylistBrowserCategory::filter() const
{
    return QString::fromLatin1( QUrl::toPercentEncoding( m_filterProxy->filterRegularExpression().pattern() ) );
}

void
PlaylistBrowserCategory::setFilter( const QString &filter )
{
    debug() << "Setting filter " << filter;
    m_filterProxy->setFilterRegularExpression( QRegularExpression( QUrl::fromPercentEncoding( filter.toUtf8() ) ) );
    //disable all other provider-buttons
    for( QAction * const providerAction : m_providerActions )
    {
        const Playlists::PlaylistProvider *provider =
                providerAction->data().value<const Playlists::PlaylistProvider *>();
        if( provider )
            providerAction->setChecked(
                    QRegularExpression(QRegularExpression::anchoredPattern(m_filterProxy->filterRegularExpression().pattern()))
                    .match( provider->prettyName() ).hasMatch());
    }
}

QTreeView *
PlaylistBrowserCategory::playlistView()
{
    return m_playlistView;
}

void
PlaylistBrowserCategory::toggleView( bool merged )
{
    if( merged )
    {
        m_filterProxy->setSourceModel( m_byFolderProxy );
        m_playlistView->setItemDelegate( m_defaultItemDelegate );
        m_playlistView->setRootIsDecorated( true );
        setHelpText( m_addFolderAction->text(), m_addFolderAction );
    }
    else
    {
        m_filterProxy->setSourceModel( m_byProviderProxy );
        m_playlistView->setItemDelegate( m_byProviderDelegate );
        m_playlistView->setRootIsDecorated( false );
        setHelpText( i18n( "Folders are only shown in <b>merged view</b>." ), m_addFolderAction );
    }

    //folders don't make sense in per-provider view
    m_addFolderAction->setEnabled( merged );

    Amarok::config( m_configGroup ).writeEntry( s_mergeViewKey, merged );
}

void
PlaylistBrowserCategory::setHelpText(const QString &text, QAction *qa)
{
    qa->setStatusTip(text);
    qa->setToolTip(text);
    if ((qa->whatsThis()).isEmpty()) {
        qa->setWhatsThis(text);
    }
}

void
PlaylistBrowserCategory::slotProviderAdded( Playlists::PlaylistProvider *provider, int category )
{
    if( category != m_playlistCategory )
        return; //ignore

    if( !m_providerActions.keys().contains( provider ) )
        createProviderButton( provider );

    if( provider->playlistCount() != 0 )
        toggleView( false ); // set view to non-merged if new provider has some tracks

    slotToggleProviderButton();
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
    connect( providerToggle, &QAction::toggled, this, &PlaylistBrowserCategory::slotToggleProviderButton );
    m_providerMenu->addAction( providerToggle );

    //if there is only one provider the button needs to be disabled.
    //When a second is added we can enable the first.
    if( m_providerActions.isEmpty() )
        providerToggle->setEnabled( false );
    else if( m_providerActions.count() == 1 )
        m_providerActions.values().first()->setEnabled( true );

    m_providerActions.insert( provider, providerToggle );
}

void
PlaylistBrowserCategory::slotToggleProviderButton()
{
    QString filter;
    QActionList checkedActions;
    for( const Playlists::PlaylistProvider *p : m_providerActions.keys() )
    {
        QAction *action = m_providerActions.value( p );
        if( action->isChecked() )
        {
            QString escapedName = QRegularExpression::escape( p->prettyName() ).replace( QLatin1Char(' '), QLatin1String("\\ ") );
            filter += QString( filter.isEmpty() ? QStringLiteral("%1") : QStringLiteral("|%1") ).arg( escapedName );
            checkedActions << action;
            action->setEnabled( true );
        }
    }
    //if all are enabled the filter can be completely disabled.
    if( checkedActions.count() == m_providerActions.count() )
        filter.clear();

    m_filterProxy->setFilterRegularExpression( filter );

    //don't allow the last visible provider to be hidden
    if( checkedActions.count() == 1 )
        checkedActions.first()->setEnabled( false );
}

void
PlaylistBrowserCategory::createNewFolder()
{
    QString name = i18nc( "default name for new folder", "New Folder" );
    const QModelIndex &rootIndex = m_byFolderProxy->index(0,0);
    QModelIndexList folderIndices = m_byFolderProxy->match( rootIndex, Qt::DisplayRole, name, -1 );
    QString groupName = name;
    if( !folderIndices.isEmpty() )
    {
        int folderCount( 0 );
        for( const QModelIndex &folder : folderIndices )
        {
            QRegularExpression regex( name + QStringLiteral(" \\((\\d+)\\)") );
            QRegularExpressionMatch rmatch = regex.match( folder.data( Qt::DisplayRole ).toString() );
            if ( rmatch.hasMatch() )
            {
                int newNumber = rmatch.captured( 1 ).toInt();
                if (newNumber > folderCount)
                    folderCount = newNumber;
            }
        }
        groupName += QStringLiteral( " (%1)" ).arg( folderCount + 1 );
    }
    QModelIndex idx = m_filterProxy->mapFromSource( m_byFolderProxy->createNewFolder( groupName ) );
    m_playlistView->setCurrentIndex( idx );
    m_playlistView->edit( idx );
}

void
PlaylistBrowserCategory::newPalette( const QPalette &palette )
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}

