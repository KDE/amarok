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
#include "PaletteHandler.h"
#include "playlist/PlaylistModel.h"
#include "PlaylistsInGroupsProxy.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "SvgHandler.h"
#include "statusbar/StatusBar.h"
#include "UserPlaylistModel.h"


#include <KAction>
#include <KIcon>
#include <KLineEdit>

#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>

#include <typeinfo>

PlaylistBrowserNS::PlaylistCategory::PlaylistCategory( QWidget * parent )
    : QWidget( parent )
{
    setContentsMargins(0,0,0,0);
    m_toolBar = new QToolBar( this );
    m_toolBar->setToolButtonStyle( Qt::ToolButtonTextBesideIcon );


    m_groupedProxy = new PlaylistsInGroupsProxy( The::userPlaylistModel() );

    m_playlistView = new UserPlaylistTreeView( m_groupedProxy, this );
//    m_playlistView = new UserPlaylistTreeView( The::userPlaylistModel(), this );
    m_playlistView->setFrameShape( QFrame::NoFrame );
    m_playlistView->setContentsMargins(0,0,0,0);
    m_playlistView->header()->hide();

    m_playlistView->setDragEnabled(true);
    m_playlistView->setAcceptDrops(true);
    m_playlistView->setDropIndicatorShown(true);

    m_playlistView->setEditTriggers( QAbstractItemView::NoEditTriggers );

    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette & ) ), SLOT( newPalette( const QPalette & ) ) );

    QVBoxLayout *vLayout = new QVBoxLayout( this );
    vLayout->setContentsMargins(0,0,0,0);
    vLayout->addWidget( m_toolBar );
    vLayout->addWidget( m_playlistView );

    m_playlistView->setAlternatingRowColors( true );

    m_addGroupAction = new KAction( KIcon("media-track-add-amarok" ), i18n( "Add Folder" ), this  );
    m_toolBar->addAction( m_addGroupAction );
//TODO:     connect( m_addGroupAction, SIGNAL( triggered( bool ) ), m_groupedProxy, SLOT( createNewGroup() ) );

    m_playlistView->setNewGroupAction( m_addGroupAction );

//     KAction* addStreamAction = new KAction( KIcon("list-add"), i18n("Add Stream"), this );
//     m_toolBar->addAction( addStreamAction );
//     connect( addStreamAction, SIGNAL( triggered( bool ) ), this, SLOT( showAddStreamDialog() ) );
}


PlaylistBrowserNS::PlaylistCategory::~PlaylistCategory()
{
}

/*
void
PlaylistBrowserNS::PlaylistCategory::showAddStreamDialog()
{
    KDialog *dialog = new PlaylistBrowserNS::treamEditor( this );
    connect( dialog, SIGNAL( okClicked() ), this, SLOT( streamDialogConfirmed() ) );
}*/
/*
void
PlaylistBrowserNS::PlaylistCategory::streamDialogConfirmed()
{
    PlaylistBrowserNS::StreamEditor* dialog = qobject_cast<PlaylistBrowserNS::StreamEditor*>( sender() );
    if( !dialog )
        return;
    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl(  dialog->streamUrl() );
    if( !track.isNull() )
    {
        m_model->createNewStream(  dialog->streamName(), track );
    }
    else
    {
        The::statusBar()->longMessage( i18n("The stream URL provided was not valid.") );
    }
}
*/

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
    connect( m_streamUrl, SIGNAL( textChanged(const QString &) ), this, SLOT( slotTextChanged( const QString& )));
    enableButtonOk( false );
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

void PlaylistBrowserNS::PlaylistCategory::newPalette(const QPalette & palette)
{
    Q_UNUSED( palette )

    The::paletteHandler()->updateItemView( m_playlistView );
}

void PlaylistBrowserNS::StreamEditor::slotTextChanged( const QString & text )
{
    enableButtonOk( !text.isEmpty() );
}






#include "PlaylistCategory.moc"


