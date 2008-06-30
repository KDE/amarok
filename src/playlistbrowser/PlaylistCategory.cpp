/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "PlaylistCategory.h"

#include "CollectionManager.h"
#include "playlist/PlaylistModel.h"
#include "SqlPlaylist.h"
#include "SqlPlaylistGroup.h"
#include "StatusBar.h"
#include "UserPlaylistModel.h"

#include <KAction>
#include <KIcon>
#include <KLineEdit>
#include <KMenu>

#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <typeinfo>

PlaylistBrowserNS::PlaylistCategory::PlaylistCategory( QWidget * parent )
    : Amarok::Widget( parent )
    , m_deleteAction( 0 )
    , m_renameAction( 0 )
    , m_addGroupAction( 0 )
{

    setContentsMargins(0,0,0,0);
    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );

    m_playlistView = new QTreeView( this );
    m_playlistView->setFrameShape( QFrame::NoFrame );
    m_playlistView->setContentsMargins(0,0,0,0);
    m_playlistView->setModel( PlaylistBrowserNS::UserModel::instance() );
    m_playlistView->header()->hide();


    m_playlistView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_playlistView->setDragEnabled(true);
    m_playlistView->setAcceptDrops(true);
    m_playlistView->setDropIndicatorShown(true);



    m_playlistView->setContextMenuPolicy( Qt::CustomContextMenu );
    m_playlistView->setEditTriggers( QAbstractItemView::NoEditTriggers );

    connect( m_playlistView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( itemActivated(  const QModelIndex & ) ) );
    connect( m_playlistView, SIGNAL( customContextMenuRequested( const QPoint & ) ), this, SLOT( showContextMenu( const QPoint & ) ) );

    connect( PlaylistBrowserNS::UserModel::instance(), SIGNAL( editIndex( const QModelIndex & ) ), m_playlistView, SLOT( edit( const QModelIndex & ) ) );

    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->addWidget( m_toolBar );
    vLayout->addWidget( m_playlistView );

    m_playlistView->setAlternatingRowColors( true );

    //transparency
    QPalette p = m_playlistView->palette();
    QColor c = p.color( QPalette::Base );

    //Give line edits a solid background color as any edit delegates will otherwise inherit the transparent base color,
    //which is bad as the line edit is drawn on top of the original name, leading to double text while editing....
    m_playlistView->setStyleSheet("QLineEdit { background-color: " + c.name() + " }");

    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );

    c = p.color( QPalette::AlternateBase );
    c.setAlpha( 77 );
    p.setColor( QPalette::AlternateBase, c );

    m_playlistView->setPalette( p );

    m_addGroupAction = new KAction( KIcon("media-track-add-amarok" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( m_addGroupAction );
    connect( m_addGroupAction, SIGNAL( triggered( bool ) ), PlaylistBrowserNS::UserModel::instance(), SLOT( createNewGroup() ) );

    KAction* addStreamAction = new KAction( KIcon("list-add"), i18n("Add Stream"), this );
    m_toolBar->addAction( addStreamAction );
    connect( addStreamAction, SIGNAL( triggered( bool ) ), this, SLOT( showAddStreamDialog() ) );
}


PlaylistBrowserNS::PlaylistCategory::~PlaylistCategory()
{
}

void PlaylistBrowserNS::PlaylistCategory::itemActivated(const QModelIndex & index)
{
    //DEBUG_BLOCK
    if ( !index.isValid() )
        return;

    SqlPlaylistViewItemPtr item =  PlaylistBrowserNS::UserModel::instance()->data( index, 0xf00d ).value<SqlPlaylistViewItemPtr>();

    if ( typeid( * item ) == typeid( Meta::SqlPlaylist ) ) {
        Meta::SqlPlaylistPtr playlist = Meta::SqlPlaylistPtr::staticCast( item );
        //debug() << "playlist name: " << playlist->name();
        //The::playlistModel()->insertOptioned( Meta::PlaylistPtr( playlist ), Playlist::Append );
        The::playlistModel()->insertOptioned( playlist->tracks(), Playlist::Append );
    }
}

void
PlaylistBrowserNS::PlaylistCategory::showContextMenu( const QPoint & pos )
{

    QModelIndexList indices = m_playlistView->selectionModel()->selectedIndexes();

    KMenu menu;

    if ( m_deleteAction == 0 )
        m_deleteAction = new KAction( KIcon("media-track-remove-amarok" ), i18n( "Delete" ), this  );

    if ( m_renameAction == 0 )
        m_renameAction = new KAction( KIcon("media-track-edit-amarok" ), i18n( "Rename" ), this  );

    if ( indices.count() > 0 )
        menu.addAction( m_deleteAction );

    if ( indices.count() == 1 )
        menu.addAction( m_renameAction );

    menu.addSeparator();
    menu.addAction( m_addGroupAction );


    KAction* result = dynamic_cast< KAction* > ( menu.exec( m_playlistView->mapToGlobal( pos ) ) );
    if ( result == 0 ) return;


    if( result == m_deleteAction )
    {
        foreach( const QModelIndex &idx, indices )
        {
            SqlPlaylistViewItemPtr item = PlaylistBrowserNS::UserModel::instance()->data( idx, 0xf00d ).value<SqlPlaylistViewItemPtr>();
            debug() << "deleting " << item->name();
            item->removeFromDb();
            item->parent()->deleteChild( item );
        }
        PlaylistBrowserNS::UserModel::instance()->reloadFromDb();
    }
    else if( result == m_renameAction )
    {
        m_playlistView->edit( indices.first() );
    }
    else if( result == m_addGroupAction )
    {
        PlaylistBrowserNS::UserModel::instance()->createNewGroup();
    }
}

void
PlaylistBrowserNS::PlaylistCategory::showAddStreamDialog()
{
    KDialog *dialog = new PlaylistBrowserNS::StreamEditor( this );
    connect( dialog, SIGNAL( okClicked() ), this, SLOT( streamDialogConfirmed() ) );
}

void
PlaylistBrowserNS::PlaylistCategory::streamDialogConfirmed()
{
    PlaylistBrowserNS::StreamEditor* dialog = qobject_cast<PlaylistBrowserNS::StreamEditor*>( sender() );
    if( !dialog )
        return;
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl(  dialog->streamUrl() );
    if( !track.isNull() )
    {
        PlaylistBrowserNS::UserModel::instance()->createNewStream(  dialog->streamName(), track );
    }
    else
    {
        The::statusBar()->longMessage( i18n("The stream URL provided was not valid.") );
    }
}

PlaylistBrowserNS::StreamEditor::StreamEditor( QWidget* parent )
    : KDialog( parent )
    , m_mainWidget( new QWidget( this ) )
    , m_streamName( new KLineEdit( m_mainWidget ) )
    , m_streamUrl( new KLineEdit( m_mainWidget ) )
{
    setCaption( i18n("Add Stream Location") );
    setButtons( KDialog::Ok | KDialog::Cancel );
    QGridLayout* layout = new QGridLayout();
    layout->addWidget( new QLabel( i18n("Name:"), m_mainWidget ), 0, 0 );
    layout->addWidget( m_streamName, 0, 1 );
    layout->addWidget( new QLabel( i18n("Stream URL:"), m_mainWidget ), 1, 0 );
    layout->addWidget( m_streamUrl, 1, 1 );
    m_mainWidget->setLayout( layout );
    setMainWidget( m_mainWidget );
    connect( this, SIGNAL( closeClicked() ), this, SLOT( delayedDestruct() ) );
    connect( this, SIGNAL( hidden() ), this, SLOT( delayedDestruct() ) );
    connect( this, SIGNAL( cancelClicked() ), this, SLOT( delayedDestruct() ) );
    show();
}

QString 
PlaylistBrowserNS::StreamEditor::streamName()
{
    return m_streamName->text().trimmed();
}

QString
PlaylistBrowserNS::StreamEditor::streamUrl()
{
    return m_streamUrl->text().trimmed();
}


#include "PlaylistCategory.moc"


