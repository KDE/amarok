/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "Playlist::Model"

#include "PlaylistModel.h"

#include "core/support/Amarok.h"
#include "SvgHandler.h"
#include "amarokconfig.h"
#include "AmarokMimeData.h"
#include "core/capabilities/ReadLabelCapability.h"
#include "core/support/Debug.h"
#include "DirectoryLoader.h"
#include "EngineController.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/collections/Collection.h"
#include "core/meta/support/MetaUtility.h"
#include "PlaylistColumnNames.h"
#include "PlaylistActions.h"
#include "PlaylistModelStack.h"
#include "PlaylistItem.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "UndoCommands.h"

#include <KGlobal>
#include <KUrl>
#include <KIconLoader>

#include <QAction>
#include <QDate>
#include <QStringList>
#include <QTextDocument>

#include <typeinfo>

bool Playlist::Model::s_tooltipColumns[NUM_COLUMNS];

// ------- helper functions for the tooltip

static QString
breakLongLinesHTML(const QString& text)
{
    // Now let's break up long lines so that the tooltip doesn't become hideously large

    // The size of the normal, standard line
    const int lnSize = 50;
    if (text.size() <= lnSize)
    {
        // If the text is not too long, return it as it is
        return text;
    }
    else
    {
        QString textInLines;

        QStringList words = text.trimmed().split(' ');
        int lineLength = 0;
        while(words.size() > 0)
        {
            QString word = words.first();
            // Let's check if the next word makes the current line too long.
            if (lineLength + word.size() + 1 > lnSize)
            {
                if (lineLength > 0)
                {
                    textInLines += "<br/>";
                }
                lineLength = 0;
                // Let's check if the next word is not too long for the new line to contain
                // If it is, cut it
                while (word.size() > lnSize)
                {
                    QString wordPart = word;
                    wordPart.resize(lnSize);
                    word.remove(0,lnSize);
                    textInLines += wordPart + "<br/>";
                }
            }
            textInLines += word + " ";
            lineLength += word.size() + 1;
            words.removeFirst();
        }
        return textInLines.trimmed();
    }
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The colunm used to display the icon
* @param value The QString value to be shown
* @return The line to be shown or an empty QString if the value is null
*/
static QString
HTMLLine( const Playlist::Column& column, const QString& value, bool force = false )
{
    if ( (!value.isEmpty()) || (force) )
    {
        QString line;
        line += "<tr><td align=\"right\">";
        line += "<img src=\""+KIconLoader::global()->iconPath( Playlist::iconNames[column] , -16)+"\" />";
        line += "</td><td align=\"left\">";
        line += breakLongLinesHTML( value );
        line += "</td></tr>";
        return line;
    }
    else
        return QString();
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The colunm used to display the icon
* @param value The integer value to be shown
* @return The line to be shown or an empty QString if the value is 0
*/
static QString
HTMLLine( const Playlist::Column& column, const int value, bool force = false )
{
    if ( (value != 0) || (force) )
    {
        return HTMLLine( column, QString::number( value ) );
    }
    else
        return QString();
}


Playlist::Model::Model( QObject *parent )
        : QAbstractListModel( parent )
        , m_activeRow( -1 )
        , m_totalLength( 0 )
        , m_totalSize( 0 )
        , m_setStateOfItem_batchMinRow( -1 )
{
    DEBUG_BLOCK
}

Playlist::Model::~Model()
{
    DEBUG_BLOCK

    // Save current playlist
    exportPlaylist( defaultPlaylistPath() );

    qDeleteAll( m_items );
}

QVariant
Playlist::Model::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role != Qt::DisplayRole )
        return QVariant();

    return columnNames( section );
}

void
Playlist::Model::setTooltipColumns( bool columns[] )
{
    for( int i=0; i<Playlist::NUM_COLUMNS; ++i )
        s_tooltipColumns[i] = columns[i];
}

QString
Playlist::Model::tooltipFor( Meta::TrackPtr track ) const
{
    QString text;
    // get the shared pointers now to be thread safe
    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumPtr album = track->album();
    Meta::GenrePtr genre = track->genre();
    Meta::ComposerPtr composer = track->composer();
    Meta::YearPtr year = track->year();

    if( s_tooltipColumns[Playlist::Title] )
        text += HTMLLine( Playlist::Title, track->prettyName() );

    if( s_tooltipColumns[Playlist::Artist] && artist )
        text += HTMLLine( Playlist::Artist, artist->prettyName() );

    if( s_tooltipColumns[Playlist::Album] && album )
        text += HTMLLine( Playlist::Album, album->prettyName() );

    if( s_tooltipColumns[Playlist::DiscNumber] )
        text += HTMLLine( Playlist::DiscNumber, track->discNumber() );

    if( s_tooltipColumns[Playlist::TrackNumber] )
        text += HTMLLine( Playlist::TrackNumber, track->trackNumber() );

    if( s_tooltipColumns[Playlist::Composer] && composer )
        text += HTMLLine( Playlist::Composer, composer->prettyName() );

    if( s_tooltipColumns[Playlist::Genre] && genre )
        text += HTMLLine( Playlist::Genre, genre->prettyName() );

    if( s_tooltipColumns[Playlist::Year] && year )
        text += HTMLLine( Playlist::Year, year->name().toInt() );

    if( s_tooltipColumns[Playlist::Comment])
        text += HTMLLine( Playlist::Comment, track->comment() );

    if( s_tooltipColumns[Playlist::Score] )
        text += HTMLLine( Playlist::Score, QString::number( static_cast<int>( track->score() ) ), true );

    if( s_tooltipColumns[Playlist::Rating] )
        text += HTMLLine( Playlist::Rating, QString::number( static_cast<double>(track->rating())/2.0 ), true );

    if( s_tooltipColumns[Playlist::PlayCount] )
        text += HTMLLine( Playlist::PlayCount, track->playCount(), true );

    if( text.isEmpty() )
        text = QString( i18n( "No extra information available" ) );
    else
        text = QString("<table>"+ text +"</table>");

    return text;
}

QVariant
Playlist::Model::data( const QModelIndex& index, int role ) const
{
    int row = index.row();

    if ( !index.isValid() || !rowExists( row ) )
        return QVariant();

    if ( role == UniqueIdRole )
        return QVariant( idAt( row ) );

    else if ( role == ActiveTrackRole )
        return ( row == m_activeRow );

    else if ( role == TrackRole && m_items.at( row )->track() )
        return QVariant::fromValue( m_items.at( row )->track() );

    else if ( role == StateRole )
        return m_items.at( row )->state();

    else if ( role == QueuePositionRole )
        return Actions::instance()->queuePosition( idAt( row ) ) + 1;

    else if ( role == InCollectionRole )
        return  m_items.at( row )->track()->inCollection();

    else if ( role == MultiSourceRole )
        return  m_items.at( row )->track()->hasCapabilityInterface( Capabilities::Capability::MultiSource );

    else if ( role == StopAfterTrackRole )
        return Actions::instance()->willStopAfterTrack( idAt( row ) );

    else if ( role == Qt::ToolTipRole )
        return tooltipFor( m_items.at( row )->track() );

    else if ( role == Qt::DisplayRole )
    {
        Meta::AlbumPtr album = m_items.at( row )->track()->album();
        switch ( index.column() )
        {
            case PlaceHolder:
                return "Test";
            case Album:
            {
                if( album )
                    return album->name();
                return QString();
            }
            case AlbumArtist:
            {
                if( album && album->albumArtist() )
                    return album->albumArtist()->name();
                return QString();
            }
            case Artist:
            {
                if ( m_items.at( row )->track()->artist() )
                    return m_items.at( row )->track()->artist()->name();
                return QString();
            }
            case Bitrate:
            {
                return Meta::prettyBitrate( m_items.at( row )->track()->bitrate() );
            }
            case Bpm:
            {
                if ( m_items.at( row )->track()->bpm() > 0.0 )
                    return QString::number( m_items.at( row )->track()->bpm() );
                return QString();
            }
            case Comment:
            {
                return m_items.at( row )->track()->comment();
            }
            case Composer:
            {
                if ( m_items.at( row )->track()->composer() )
                    return m_items.at( row )->track()->composer()->name();
                return QString();
            }
            case CoverImage:
            {
                if( album )
                    return The::svgHandler()->imageWithBorder( album, 100 ); //FIXME:size?
                return QImage();
            }
            case Directory:
            {
                if ( m_items.at( row )->track()->playableUrl().isLocalFile() )
                    return m_items.at( row )->track()->playableUrl().directory();
                else
                    return QString();
            }
            case DiscNumber:
            {
                return m_items.at( row )->track()->discNumber();
            }
            case Filename:
            {

                if ( m_items.at( row )->track()->playableUrl().isLocalFile() )
                    return m_items.at( row )->track()->playableUrl().fileName();
                else
                    return QString();
            }
            case Filesize:
            {
                return Meta::prettyFilesize( m_items.at( row )->track()->filesize() );
            }
            case Genre:
            {
                if ( m_items.at( row )->track()->genre() )
                    return m_items.at( row )->track()->genre()->name();
                return QString();
            }
            case GroupLength:
            {
                return Meta::secToPrettyTime( 0 );
            }
            case GroupTracks:
            {
                return QString();
            }
            case Labels:
            {
                Meta::TrackPtr track = m_items.at( row )->track();
                if( track )
                {
                    QStringList labelNames;
                    foreach( const Meta::LabelPtr &label, track->labels() )
                    {
                        labelNames << label->prettyName();
                    }
                    return labelNames.join( ", " );
                }
                return QString();
            }
            case LastPlayed:
            {
                if( m_items.at( row )->track()->lastPlayed().toTime_t() > 0 )
                    return Amarok::verboseTimeSince( m_items.at( row )->track()->lastPlayed() );
                else
                    return i18n( "Never" );
            }
            case Length:
            {
                return Meta::msToPrettyTime( m_items.at( row )->track()->length() );
            }
            case LengthInSeconds:
            {
                return m_items.at( row )->track()->length() / 1000;
            }
            case Mood:
            {
                return QString(); //FIXME
            }
            case PlayCount:
            {
                return m_items.at( row )->track()->playCount();
            }
            case Rating:
            {
                return m_items.at( row )->track()->rating();
            }
            case SampleRate:
            {
                return m_items.at( row )->track()->sampleRate();
            }
            case Score:
            {
                return (int)m_items.at( row )->track()->score(); // Cast to int, as we don't need to show the decimals in the view..
            }
            case Source:
            {
                QString sourceName;
                Capabilities::SourceInfoCapability *sic = m_items.at( row )->track()->create<Capabilities::SourceInfoCapability>();
                if ( sic )
                {
                    sourceName = sic->sourceName();
                    delete sic;
                }
                else
                {
                    sourceName = m_items.at( row )->track()->collection() ? m_items.at( row )->track()->collection()->prettyName() : QString();
                }
                return sourceName;
            }
            case SourceEmblem:
            {
                QPixmap emblem;
                Capabilities::SourceInfoCapability *sic = m_items.at( row )->track()->create<Capabilities::SourceInfoCapability>();
                if ( sic )
                {
                    QString source = sic->sourceName();
                    if ( !source.isEmpty() )
                        emblem = sic->emblem();
                    delete sic;
                }
                return emblem;
            }
            case Title:
            {
                return m_items.at( row )->track()->prettyName();
            }
            case TitleWithTrackNum:
            {
                QString trackString;
                QString trackName = m_items.at( row )->track()->prettyName();
                if ( m_items.at( row )->track()->trackNumber() > 0 )
                {
                    QString trackNumber = QString::number( m_items.at( row )->track()->trackNumber() );
                    trackString =  QString( trackNumber + " - " + trackName );
                } else
                    trackString = trackName;

                return trackString;
            }
            case TrackNumber:
            {
                return m_items.at( row )->track()->trackNumber();
            }
            case Type:
            {
                return m_items.at( row )->track()->type();
            }
            case Year:
            {
                if ( m_items.at( row )->track()->year() )
                    return m_items.at( row )->track()->year()->name();
                return QString();
            }
            default:
                return QString();

        }
    }
    // else
    return QVariant();
}

Qt::DropActions
Playlist::Model::supportedDropActions() const
{
    return Qt::MoveAction | QAbstractListModel::supportedDropActions();
}

Qt::ItemFlags
Playlist::Model::flags( const QModelIndex &index ) const
{
    if ( index.isValid() )
        return ( Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable );
    return Qt::ItemIsDropEnabled;
}

QStringList
Playlist::Model::mimeTypes() const
{
    QStringList ret = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::TRACK_MIME;
    ret << "text/uri-list"; //we do accept urls
    return ret;
}

QMimeData*
Playlist::Model::mimeData( const QModelIndexList &indexes ) const
{
    AmarokMimeData* mime = new AmarokMimeData();
    Meta::TrackList selectedTracks;

    foreach( const QModelIndex &it, indexes )
    selectedTracks << m_items.at( it.row() )->track();

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Playlist::Model::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int, const QModelIndex& parent )
{
    DEBUG_BLOCK
    if ( action == Qt::IgnoreAction )
        return true;

    int beginRow;
    if ( row != -1 )
        beginRow = row;
    else if ( parent.isValid() )
        beginRow = parent.row();
    else
        beginRow = m_items.size();

    if( data->hasFormat( AmarokMimeData::TRACK_MIME ) )
    {
        debug() << "this is a track";
        const AmarokMimeData* trackListDrag = qobject_cast<const AmarokMimeData*>( data );
        if( trackListDrag )
        {

            Meta::TrackList tracks = trackListDrag->tracks();
            qStableSort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

            The::playlistController()->insertTracks( beginRow, tracks );
        }
        return true;
    }
    else if( data->hasFormat( AmarokMimeData::PLAYLIST_MIME ) )
    {
        debug() << "this is a playlist";
        const AmarokMimeData* dragList = qobject_cast<const AmarokMimeData*>( data );
        if( dragList )
            The::playlistController()->insertPlaylists( beginRow, dragList->playlists() );
        return true;
    }
    else if( data->hasFormat( AmarokMimeData::PODCASTEPISODE_MIME ) )
    {
        debug() << "this is a podcast episode";
        const AmarokMimeData* dragList = qobject_cast<const AmarokMimeData*>( data );
        if( dragList )
        {
            Meta::TrackList tracks;
            foreach( Podcasts::PodcastEpisodePtr episode, dragList->podcastEpisodes() )
                tracks << Meta::TrackPtr::staticCast( episode );
            The::playlistController()->insertTracks( beginRow, tracks );
        }
        return true;
    }
    else if( data->hasFormat( AmarokMimeData::PODCASTCHANNEL_MIME ) )
    {
        debug() << "this is a podcast channel";
        const AmarokMimeData* dragList = qobject_cast<const AmarokMimeData*>( data );
        if( dragList )
        {
            Meta::TrackList tracks;
            foreach( Podcasts::PodcastChannelPtr channel, dragList->podcastChannels() )
                foreach( Podcasts::PodcastEpisodePtr episode, channel->episodes() )
                    tracks << Meta::TrackPtr::staticCast( episode );
            The::playlistController()->insertTracks( beginRow, tracks );
        }
        return true;
    }
    else if( data->hasUrls() )
    {
        debug() << "this is _something_ with a url....";
        DirectoryLoader* dl = new DirectoryLoader(); //this deletes itself
        dl->insertAtRow( beginRow );
        dl->init( data->urls() );
        return true;
    }

    debug() << "I have no idea what the hell this is...";
    return false;
}

void
Playlist::Model::setActiveRow( int row )
{
    if ( rowExists( row ) )
    {
        setStateOfRow( row, Item::Played );
        m_activeRow = row;
        emit activeTrackChanged( m_items.at( row )->id() );
    }
    else
    {
        m_activeRow = -1;
        emit activeTrackChanged( 0 );
    }
}

void
Playlist::Model::setRowQueued( int row )
{
    if( rowExists( row ) )
    {
        Item::State state = stateOfRow(row);
        if( state == Item::Invalid )
            state = Item::Queued;
        else
            state = (Item::State) ( state | Item::Queued );
        setStateOfRow( row, state );
        emit queueChanged();
    }
}

void
Playlist::Model::setRowDequeued( int row )
{
    if( rowExists( row ) )
    {
        Item::State state = stateOfRow(row);
        if( state == Item::Queued )
            state = Item::Invalid;
        else
            state = (Item::State) ( stateOfRow(row) & ~Item::Queued );
        setStateOfRow( row, state );
        emit queueChanged();
    }
}

Playlist::Item::State
Playlist::Model::stateOfRow( int row ) const
{
    if ( rowExists( row ) )
        return m_items.at( row )->state();
    else
        return Item::Invalid;
}

bool
Playlist::Model::containsTrack( const Meta::TrackPtr track ) const
{
    foreach( Item* i, m_items )
    {
        if ( i->track() == track )
            return true;
    }
    return false;
}

int
Playlist::Model::firstRowForTrack( const Meta::TrackPtr track ) const
{
    int row = 0;
    foreach( Item* i, m_items )
    {
        if ( i->track() == track )
            return row;
        row++;
    }
    return -1;
}

QSet<int>
Playlist::Model::allRowsForTrack( const Meta::TrackPtr track ) const
{
    QSet<int> trackRows;

    int row = 0;
    foreach( Item* i, m_items )
    {
        if ( i->track() == track )
            trackRows.insert( row );
        row++;
    }
    return trackRows;
}

Meta::TrackPtr
Playlist::Model::trackAt( int row ) const
{
    if ( rowExists( row ) )
        return m_items.at( row )->track();
    else
        return Meta::TrackPtr();
}

Meta::TrackPtr
Playlist::Model::activeTrack() const
{
    if ( rowExists( m_activeRow ) )
        return m_items.at( m_activeRow )->track();
    else
        return Meta::TrackPtr();
}

int
Playlist::Model::rowForId( const quint64 id ) const
{
    return m_items.indexOf( m_itemIds.value( id ) );    // Returns -1 on miss, same as our API.
}

Meta::TrackPtr
Playlist::Model::trackForId( const quint64 id ) const
{
    Item* item = m_itemIds.value( id, 0 );
    if ( item )
        return item->track();
    else
        return Meta::TrackPtr();
}

quint64
Playlist::Model::idAt( const int row ) const
{
    if ( rowExists( row ) )
        return m_items.at( row )->id();
    else
        return 0;
}

quint64
Playlist::Model::activeId() const
{
    if ( rowExists( m_activeRow ) )
        return m_items.at( m_activeRow )->id();
    else
        return 0;
}

Playlist::Item::State
Playlist::Model::stateOfId( quint64 id ) const
{
    Item* item = m_itemIds.value( id, 0 );
    if ( item )
        return item->state();
    else
        return Item::Invalid;
}

void
Playlist::Model::metadataChanged( Meta::TrackPtr track )
{
    DEBUG_BLOCK

    const int size = m_items.size();
    for ( int i = 0; i < size; i++ )
    {
        if ( m_items.at( i )->track() == track )
        {
            emit dataChanged( index( i, 0 ), index( i, columnCount() - 1 ) );
            debug()<<"Metadata updated for track"<<track->prettyName();
            break;
        }
    }
}

void
Playlist::Model::metadataChanged( Meta::AlbumPtr album )
{
    // Mainly to get update about changed covers
    const int size = m_items.size();
    for ( int i = 0; i < size; i++ )
    {
        if ( m_items.at( i )->track()->album() == album )
        {
            emit dataChanged( index( i, 0 ), index( i, columnCount() - 1 ) );
            debug()<<"Metadata updated for album"<<album->prettyName();
            break;
        }
    }
}

bool
Playlist::Model::exportPlaylist( const QString &path ) const
{
    // check queue state
    QQueue<quint64> queueIds = The::playlistActions()->queue();
    QList<int> queued;
    foreach( quint64 id, queueIds ) {
      queued << rowForId( id );
    }
    return Playlists::exportPlaylistFile( tracks(), path, queued );
}

Meta::TrackList
Playlist::Model::tracks() const
{
    Meta::TrackList tl;
    foreach( Item* item, m_items )
        tl << item->track();
    return tl;
}

QString
Playlist::Model::prettyColumnName( Column index ) //static
{
    switch ( index )
    {
    case Filename:   return i18nc( "The name of the file this track is stored in", "Filename" );
    case Title:      return i18n( "Title" );
    case Artist:     return i18n( "Artist" );
    case AlbumArtist: return i18n( "Album Artist" );
    case Composer:   return i18n( "Composer" );
    case Year:       return i18n( "Year" );
    case Album:      return i18n( "Album" );
    case DiscNumber: return i18n( "Disc Number" );
    case TrackNumber: return i18nc( "The Track number for this item", "Track" );
    case Bpm:        return i18n( "BPM" );
    case Genre:      return i18n( "Genre" );
    case Comment:    return i18n( "Comment" );
    case Directory:  return i18nc( "The location on disc of this track", "Directory" );
    case Type:       return i18n( "Type" );
    case Length:     return i18n( "Length" );
    case Bitrate:    return i18n( "Bitrate" );
    case SampleRate: return i18n( "Sample Rate" );
    case Score:      return i18n( "Score" );
    case Rating:     return i18n( "Rating" );
    case PlayCount:  return i18n( "Play Count" );
    case LastPlayed: return i18nc( "Column name", "Last Played" );
    case Mood:       return i18n( "Mood" );
    case Filesize:   return i18n( "File Size" );
    default:         return "This is a bug.";
    }

}


////////////
//Private Methods
////////////

void
Playlist::Model::insertTracksCommand( const InsertCmdList& cmds )
{
    if ( cmds.size() < 1 )
        return;

    setAllNewlyAddedToUnplayed();

    int activeShift = 0;
    int min = m_items.size() + cmds.size();
    int max = 0;
    int begin = cmds.at( 0 ).second;
    foreach( const InsertCmd &ic, cmds )
    {
        min = qMin( min, ic.second );
        max = qMax( max, ic.second );
        activeShift += ( begin <= m_activeRow ) ? 1 : 0;
    }

    // actually do the insertion
    beginInsertRows( QModelIndex(), min, max );
    foreach( const InsertCmd &ic, cmds )
    {
        Meta::TrackPtr track = ic.first;
        m_totalLength += track->length();
        m_totalSize += track->filesize();
        subscribeTo( track );
        Meta::AlbumPtr album = track->album();
        if( album )
            subscribeTo( album );

        Item* newitem = new Item( track );
        m_items.insert( ic.second, newitem );
        m_itemIds.insert( newitem->id(), newitem );
    }
    endInsertRows();

    if( m_activeRow >= 0 )
        m_activeRow += activeShift;
    else
    {
         // If one of the inserted tracks is currently playing, choose it as the active track.
        const Meta::TrackPtr engineTrack = The::engineController()->currentTrack();
        if( engineTrack )
        {
            int engineRow = firstRowForTrack( engineTrack );
            if( engineRow > -1 )
                setActiveRow( engineRow );
        }
    }
}


void
Playlist::Model::removeTracksCommand( const RemoveCmdList& cmds )
{
    DEBUG_BLOCK
    if ( cmds.size() < 1 )
        return;

    if ( cmds.size() == m_items.size() )
    {
        clearCommand();
        return;
    }

    int activeShift = 0;
    bool activeDeleted = false;
    foreach( const RemoveCmd &rc, cmds )
    {
        activeShift += ( rc.second < m_activeRow ) ? 1 : 0;
        if ( rc.second == m_activeRow )
            activeDeleted = true;
    }

    /* This next bit is probably more complicated that you expected it to be.
     * The reason for the complexity comes from the following:
     *
     * 1. Qt's Model/View architecture can handle removal of only consecutive rows
     * 2. The "remove rows" command from the Controller must handle
     *    non-consecutive rows, and the removal command probably isn't sorted
     *
     * So each item has to be removed individually, and you can't just iterate
     * over the commands, calling "m_items.removeAt(index)" as you go, because
     * the indices of m_items will change with every removeAt().  Thus the
     * following strategy of copying m_item, looking up the index in the copy and
     * removing the item from the original list. (The strategy was changed from
     * only replacing m_items at the end because that meant lying to the view -- Max)
     *
     * As a safety measure, the items themselves are not deleted until after m_items
     * has been replaced.  If you delete as you go, then m_items will be holding
     * dangling pointers, and the program will probably crash if the model is
     * accessed in this state.   -- stharward */

    QList<Item*> originalList(m_items); // copy the current item list
    QList<Item*> delitems;
    foreach( const RemoveCmd &rc, cmds )
    {
        Meta::TrackPtr track = rc.first;
        Item* item = originalList.at(rc.second);
        Q_ASSERT( track == item->track() );

        // -- remove the items from the lists
        int idx = rowForItem( item );
        if (idx != -1) {
            beginRemoveRows(QModelIndex(), idx, idx);
            delitems.append(item);
            m_items.removeAll( item );
            m_itemIds.remove( item->id() );
            endRemoveRows();
        } else {
            error() << "tried to delete a non-existent item:" << rc.first->prettyName() << rc.second;
        }

        m_totalLength -= track->length();
        m_totalSize -= track->filesize();

        // -- unsubscribe
        if( !containsTrack( track ) ) // check against same track two times in playlist
        {
            unsubscribeFrom( track );

            Meta::AlbumPtr album = track->album();
            if( album )
            {
                // check if we are the last track in playlist with this album
                bool last = true;
                const int size = m_items.size();
                for ( int i = 0; i < size; i++ )
                {
                    if( m_items.at( i )->track()->album() == album )
                    {
                        last = false;
                        break;
                    }
                }
                if( last )
                    unsubscribeFrom( album );
            }
        }
    }

    qDeleteAll(delitems);
    delitems.clear();

    //update the active row
    if ( !activeDeleted && ( m_activeRow >= 0 ) )
    {
        m_activeRow = ( m_activeRow > 0 ) ? m_activeRow - activeShift : 0;
    }
    else
    {
        m_activeRow = -1;
    }

    //make sure that there are enough tracks if we just removed from a dynamic playlist.
    Playlist::Actions::instance()->normalizeDynamicPlaylist();
}


void Playlist::Model::clearCommand()
{
    setActiveRow( -1 );

    beginRemoveRows( QModelIndex(), 0, rowCount() - 1 );

    m_totalLength = 0;
    m_totalSize = 0;

    qDeleteAll( m_items );
    m_items.clear();
    m_itemIds.clear();

    endRemoveRows();
}


// Note: this function depends on 'MoveCmdList' to be a complete "cycle", in the sense
// that if row A is moved to row B, another row MUST be moved to row A.
// Very strange API design IMHO, because it forces our caller to e.g. move ALL ROWS in
// the playlist to move row 0 to the last row. This function should just have been
// equivalent to a 'removeTracks()' followed by an 'insertTracks()' IMHO.  --Nanno

void
Playlist::Model::moveTracksCommand( const MoveCmdList& cmds, bool reverse )
{
    DEBUG_BLOCK
    debug()<<"moveTracksCommand:"<<cmds.size()<<reverse;

    if ( cmds.size() < 1 )
        return;

    int min = m_items.size() + cmds.size();
    int max = 0;
    foreach( const MoveCmd &rc, cmds )
    {
        min = qMin( min, rc.first );
        min = qMin( min, rc.second );
        max = qMax( max, rc.first );
        max = qMax( max, rc.second );
    }

    int newActiveRow = m_activeRow;
    QList<Item*> oldItems( m_items );
    if ( reverse )
    {
        foreach( const MoveCmd &mc, cmds )
        {
            m_items[mc.first] = oldItems.at( mc.second );
            if ( m_activeRow == mc.second )
                newActiveRow = mc.first;
        }
    }
    else
    {
        foreach( const MoveCmd &mc, cmds )
        {
            m_items[mc.second] = oldItems.at( mc.first );
            if ( m_activeRow == mc.first )
                newActiveRow = mc.second;
        }
    }

    // We have 3 choices:
    //   - Qt 4.6 'beginMoveRows()' / 'endMoveRows()'. Drawback: we'd need to do N of them, all causing resorts etc.
    //   - Emit 'layoutAboutToChange' / 'layoutChanged'. Drawback: unspecific, 'changePersistentIndex()' complications.
    //   - Emit 'dataChanged'. Drawback: a bit inappropriate. But not wrong.
    emit dataChanged( index( min, 0 ), index( max, columnCount() - 1 ) );

    //update the active row
    m_activeRow = newActiveRow;
}


// When doing a 'setStateOfItem_batch', we emit 1 crude 'dataChanged' signal. If we're
// unlucky, that signal may span many innocent rows that haven't changed at all.
// Although that "worst case" will cause unnecessary work in our listeners "upstream", it
// still has much better performance than the worst case of emitting very many tiny
// 'dataChanged' signals.
//
// Being more clever (coalesce multiple contiguous ranges, etc.) is not worth the effort.
void
Playlist::Model::setStateOfItem_batchStart()
{
    m_setStateOfItem_batchMinRow = rowCount() + 1;
    m_setStateOfItem_batchMaxRow = 0;
}

void
Playlist::Model::setStateOfItem_batchEnd()
{
    if ( ( m_setStateOfItem_batchMaxRow - m_setStateOfItem_batchMinRow ) >= 0 )
        emit dataChanged( index( m_setStateOfItem_batchMinRow, 0 ), index( m_setStateOfItem_batchMaxRow, columnCount() - 1 ) );

    m_setStateOfItem_batchMinRow = -1;
}

void
Playlist::Model::setStateOfItem( Item *item, int row, Item::State state )
{
    item->setState( state );

    if ( m_setStateOfItem_batchMinRow == -1 )    // If not in batch mode
        emit dataChanged( index( row, 0 ), index( row, columnCount() - 1 ) );
    else
    {
        m_setStateOfItem_batchMinRow = qMin( m_setStateOfItem_batchMinRow, row );
        m_setStateOfItem_batchMaxRow = qMax( m_setStateOfItem_batchMaxRow, row );
    }
}


// Unimportant TODO: the performance of this function is O(n) in playlist size.
// Not a big problem, because it's called infrequently.
// Can be fixed by maintaining a new member variable 'QMultiHash<Item::State, Item*>'.
void
Playlist::Model::setAllNewlyAddedToUnplayed()
{
    DEBUG_BLOCK

    setStateOfItem_batchStart();

    for ( int row = 0; row < rowCount(); row++ )
    {
        Item* item = m_items.at( row );
        if ( item->state() == Item::NewlyAdded )
            setStateOfItem( item, row, Item::Unplayed );
    }

    setStateOfItem_batchEnd();
}

void Playlist::Model::setAllUnplayed()
{
    DEBUG_BLOCK

    setStateOfItem_batchStart();

    for ( int row = 0; row < rowCount(); row++ )
    {
        Item* item = m_items.at( row );
        setStateOfItem( item, row, Item::Unplayed );
    }

    setStateOfItem_batchEnd();
}
