/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#define DEBUG_PREFIX "MusicBrainzTagDialog"

#include "MusicBrainzTagger.h"

#include "core/meta/support/MetaConstants.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"
#include "ui_MusicBrainzTagger.h"

#include <KMessageBox>
#include <QTimer>

MusicBrainzTagger::MusicBrainzTagger( const Meta::TrackList &tracks,
                                      QWidget *parent )
    : KDialog( parent )
    , ui( new Ui::MusicBrainzTagger() )
{
    DEBUG_BLOCK
    foreach( Meta::TrackPtr track, tracks)
    {
        if( !track->playableUrl().toLocalFile().isEmpty() )
            m_tracks << track;
    }
    ui->setupUi( mainWidget() );
    resize( QSize( 640, 480 ) );

    init();
    search();
}

MusicBrainzTagger::~MusicBrainzTagger()
{
    delete mb_finder;
#ifdef HAVE_LIBOFA
    delete mdns_finder;
#endif
    delete ui;
}

void
MusicBrainzTagger::init()
{
    DEBUG_BLOCK
    setButtons( KDialog::None );
    setAttribute( Qt::WA_DeleteOnClose );

    ui->progressBar->hide();

    mb_finder = new MusicBrainzFinder( this );
    q_resultsModel = new MusicBrainzTagsModel( m_tracks, this );
    q_trackListModel = new MusicBrainzTrackListModel( m_tracks, this );
    ui->treeView_Result->setModel( q_resultsModel );
    ui->treeView_Tracks->setModel( q_trackListModel );
    ui->treeView_Result->header()->setClickable( true );
    ui->treeView_Result->header()->setResizeMode( 0, QHeaderView::Fixed );
    ui->treeView_Result->header()->resizeSection( 0, 23 );
    ui->treeView_Result->header()->setToolTip( i18n( "Click on the first column header to check all tracks" ) );

#ifdef HAVE_LIBOFA
    mdns_used = mdns_searchDone = mb_searchDone= false;
    mdns_finder = new MusicDNSFinder( this );
    connect( mdns_finder, SIGNAL( trackFound( Meta::TrackPtr, QString ) ),
             mb_finder, SLOT( lookUpByPUID( Meta::TrackPtr, QString ) ) );
    connect( mdns_finder, SIGNAL( done() ), this, SLOT( mdnsSearchDone() ) );
#endif
    connect( mb_finder, SIGNAL( trackFound( const Meta::TrackPtr, const QVariantMap ) ),
             SLOT( trackFound( const Meta::TrackPtr, const QVariantMap ) ) );
    connect( mb_finder, SIGNAL( done() ), SLOT( searchDone() ) );
    connect( mb_finder, SIGNAL( trackFound( const Meta::TrackPtr, const QVariantMap ) ),
             q_resultsModel, SLOT( addTrack( const Meta::TrackPtr, const QVariantMap ) ) );
    connect( mb_finder, SIGNAL( progressStep() ), SLOT( progressStep() ) );
    connect( ui->treeView_Result->header(), SIGNAL( sectionClicked( int ) ),
             q_resultsModel, SLOT( selectAll( int ) ) );
    connect( ui->pushButton_StartSearch, SIGNAL( clicked() ), SLOT( search() ) );
    connect( ui->pushButton_saveAndClose, SIGNAL( clicked( bool ) ), SLOT( saveAndExit() ) );
    connect( ui->pushButton_cancel, SIGNAL( clicked( bool ) ), SLOT( reject() ) );
}

void
MusicBrainzTagger::search()
{
    m_failedTracks.clear();

    if( ui->treeView_Tracks->selectionModel()->selectedRows().isEmpty() )
        ui->treeView_Tracks->selectAll();

    foreach( QModelIndex index, ui->treeView_Tracks->selectionModel()->selectedRows() )
    {
        Meta::TrackPtr track;
        if( !( track = q_trackListModel->getTrack( index ) ).isNull() )
            m_failedTracks << track;
    }
    ui->pushButton_StartSearch->setEnabled( false );
    ui->progressBar->setRange( 0, m_failedTracks.count() );
    ui->progressBar->setValue( 0 );
    ui->horizontalSpacer->changeSize( 0, 0, QSizePolicy::Ignored );
    ui->progressBar->show();
#ifdef HAVE_LIBOFA
    if( mb_searchDone )
    {
        mdns_used = true;
        mdns_finder->run( m_failedTracks );
        return;
    }
#endif
    mb_finder->run( m_failedTracks );
}

void
MusicBrainzTagger::saveAndExit()
{
    QMap < Meta::TrackPtr, QVariantMap > result, tracksToSave = q_resultsModel->getAllChecked();
    foreach( Meta::TrackPtr track, tracksToSave.keys() )
    {
        QVariantMap tags = tracksToSave.value( track );

        if( track->name() == tags.value( Meta::Field::TITLE ).toString() )
            tags.remove( Meta::Field::TITLE );

        if( !track->artist().isNull() && tags.contains( Meta::Field::ARTIST ) &&
            track->artist()->name() == tags.value( Meta::Field::ARTIST ).toString() )
            tags.remove( Meta::Field::ARTIST );

        if( !track->album().isNull() )
        {
            if( tags.contains( Meta::Field::ALBUM ) &&
                track->album()->name() == tags.value( Meta::Field::ALBUM ).toString() )
                tags.remove( Meta::Field::ALBUM );

            if( tags.contains( Meta::Field::ALBUMARTIST ) &&
                track->album()->hasAlbumArtist() && track->album()->albumArtist()->name()
                == tags.value( Meta::Field::ALBUMARTIST ).toString() )
                tags.remove( Meta::Field::ALBUMARTIST );
        }

        if( !track->year().isNull() && tags.contains( Meta::Field::YEAR ) &&
            track->year()->name() == tags.value( Meta::Field::YEAR ).toString() )
            tags.remove( Meta::Field::YEAR );

        if( tags.contains( Meta::Field::TRACKNUMBER ) &&
            track->trackNumber() == tags.value( Meta::Field::TRACKNUMBER ).toInt() )
            tags.remove( Meta::Field::TRACKNUMBER );

        if( track->uidUrl().indexOf( tags.value( Meta::Field::UNIQUEID ).toString() ) != -1 )
            tags.remove( Meta::Field::UNIQUEID );

        if( !tags.isEmpty() )
            result.insert( track, tags );
    }

    if( !result.isEmpty() )
        emit sendResult( result );

    accept();
}

void
MusicBrainzTagger::searchDone()
{
    DEBUG_BLOCK
#ifdef HAVE_LIBOFA
    if( mdns_used && !mdns_searchDone )
        return;
    if( !m_failedTracks.isEmpty() && !mdns_used )
        if( KMessageBox::questionYesNo( this, i18n( "There are tracks that MusicBrainz didn't find. \
Try to find them with MusicDNS service?" ), windowTitle() ) == KMessageBox::Yes )
        {
            mdns_used = true;
            mdns_searchDone = false;
            ui->progressBar->setRange( 0, m_failedTracks.count() );
            ui->progressBar->setValue( 0 );
            mdns_finder->run( m_failedTracks );
            return;
        }
    mb_searchDone = true;
#endif
    ui->horizontalSpacer->changeSize( 0, 0, QSizePolicy::Expanding );
    ui->progressBar->hide();
    ui->pushButton_StartSearch->setEnabled( true );
}

#ifdef HAVE_LIBOFA
void
MusicBrainzTagger::mdnsSearchDone()
{
    DEBUG_BLOCK
    mdns_searchDone = true;
    if( !mb_finder->isRunning() )
        searchDone();
}
#endif

void
MusicBrainzTagger::trackFound( const Meta::TrackPtr track, const QVariantMap tags )
{
    DEBUG_BLOCK
    Q_UNUSED( tags );
    m_failedTracks.removeOne( track );
}

void
MusicBrainzTagger::progressStep()
{
    ui->progressBar->setValue( ui->progressBar->value() + 1 );
}

#include "MusicBrainzTagger.moc"