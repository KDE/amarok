/****************************************************************************************
 * Copyright (c) 2007-2008 Ian Monroe <ian@monroe.nu>                                   *
 * Copyright (c) 2007-2009 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 * Copyright (c) 2010 Dennis Francis <dennisfrancis.in@gmail.com>                       *
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
#include "EngineController.h"
#include "core/capabilities/MultiSourceCapability.h"
#include "core/capabilities/SourceInfoCapability.h"
#include "core/collections/Collection.h"
#include "core/meta/Statistics.h"
#include "core/meta/support/MetaUtility.h"
#include "PlaylistDefines.h"
#include "PlaylistActions.h"
#include "PlaylistController.h"
#include "PlaylistItem.h"
#include "core-impl/playlists/types/file/PlaylistFileSupport.h"
#include "core-impl/support/TrackLoader.h"
#include "playlist/UndoCommands.h"

#include <KLocalizedString>
#include <KIconLoader>

#include <QAction>
#include <QTimer>
#include <QDate>
#include <QStringList>
#include <QUrl>

#include <algorithm>


#define TOOLTIP_STATIC_LINEBREAK 50

bool Playlist::Model::s_tooltipColumns[NUM_COLUMNS];
bool Playlist::Model::s_showToolTip;

// ------- helper functions for the tooltip

static bool
fitsInOneLineHTML(const QString& text)
{
    // The size of the normal, standard line
    const int lnSize = TOOLTIP_STATIC_LINEBREAK;
    return (text.size() <= lnSize);
}

static QString
breakLongLinesHTML( const QString &origText )
{
    // filter-out HTML tags..
    QString text( origText );
    text.replace( QLatin1Char('&'), QLatin1String("&amp;") ); // needs to be first, obviously
    text.replace( QLatin1Char('<'), QLatin1String("&lt;") );
    text.replace( QLatin1Char('>'), QLatin1String("&gt;") );

    // Now let's break up long lines so that the tooltip doesn't become hideously large
    if( fitsInOneLineHTML( text ) )
        // If the text is not too long, return it as it is
        return text;
    else
    {
        const int lnSize = TOOLTIP_STATIC_LINEBREAK;
        QString textInLines;

        QStringList words = text.trimmed().split(QLatin1Char(' '));
        int lineLength = 0;
        while(words.size() > 0)
        {
            QString word = words.first();
            // Let's check if the next word makes the current line too long.
            if (lineLength + word.size() + 1 > lnSize)
            {
                if (lineLength > 0)
                {
                    textInLines += QLatin1String("<br/>");
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
            textInLines += word + ' ';
            lineLength += word.size() + 1;
            words.removeFirst();
        }
        return textInLines.trimmed();
    }
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The column used to display the icon
* @param value The QString value to be shown
* @param force If @c true, allows to set empty values
* @return The line to be shown or an empty QString if the value is null
*/
static QString
HTMLLine( const Playlist::Column& column, const QString& value, bool force = false )
{
    if( !value.isEmpty() || force )
    {
        QString line;
        line += QLatin1String("<tr><td align=\"right\">");
        line += "<img src=\""+KIconLoader::global()->iconPath( Playlist::iconName( column ), -16)+"\" />";
        line += QLatin1String("</td><td align=\"left\">");
        line += breakLongLinesHTML( value );
        line += QLatin1String("</td></tr>");
        return line;
    }
    else
        return QString();
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The column used to display the icon
* @param value The integer value to be shown
* @param force If @c true, allows to set non-positive values
* @return The line to be shown or an empty QString if the value is 0
*/
static QString
HTMLLine( const Playlist::Column& column, const int value, bool force = false )
{
    // there is currently no numeric meta-data that would have sense if it were negative.
    // also, zero denotes not available, unknown etc; don't show these unless forced.
    if( value > 0 || force )
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
        , m_saveStateTimer( new QTimer(this) )
{
    DEBUG_BLOCK

    m_saveStateTimer->setInterval( 5000 );
    m_saveStateTimer->setSingleShot( true );
    connect( m_saveStateTimer, &QTimer::timeout,
             this, &Playlist::Model::saveState );
    connect( this, &Playlist::Model::modelReset,
             this, &Playlist::Model::queueSaveState );
    connect( this, &Playlist::Model::dataChanged,
             this, &Playlist::Model::queueSaveState );
    connect( this, &Playlist::Model::rowsInserted,
             this, &Playlist::Model::queueSaveState );
    connect( this, &Playlist::Model::rowsMoved,
             this, &Playlist::Model::queueSaveState );
    connect( this, &Playlist::Model::rowsRemoved,
             this, &Playlist::Model::queueSaveState );
}

Playlist::Model::~Model()
{
    DEBUG_BLOCK

    // Save current playlist
    exportPlaylist( Amarok::defaultPlaylistPath() );

    qDeleteAll( m_items );
}

void
Playlist::Model::saveState()
{
    exportPlaylist( Amarok::defaultPlaylistPath() );
}

void
Playlist::Model::queueSaveState()
{
    if ( !m_saveStateTimer->isActive() )
        m_saveStateTimer->start();
}

void
Playlist::Model::insertTracksFromTrackLoader( const Meta::TrackList &tracks )
{
    QObject *loader = sender();
    if( !sender() )
    {
        warning() << __PRETTY_FUNCTION__ << "can only be connected to TrackLoader";
        return;
    }
    int insertRow = loader->property( "beginRow" ).toInt();
    Controller::instance()->insertTracks( insertRow, tracks );
}

QVariant
Playlist::Model::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role != Qt::DisplayRole )
        return QVariant();

    return columnName( static_cast<Playlist::Column>( section ) );
}

void
Playlist::Model::setTooltipColumns( bool columns[] )
{
    for( int i=0; i<Playlist::NUM_COLUMNS; ++i )
        s_tooltipColumns[i] = columns[i];
}

void
Playlist::Model::enableToolTip( bool enable )
{
    s_showToolTip = enable;
}

QString
Playlist::Model::tooltipFor( Meta::TrackPtr track ) const
{
    QString text;
    // get the shared pointers now to be thread safe
    Meta::ArtistPtr artist = track->artist();
    Meta::AlbumPtr album = track->album();
    Meta::ArtistPtr albumArtist = album ? album->albumArtist() : Meta::ArtistPtr();
    Meta::GenrePtr genre = track->genre();
    Meta::ComposerPtr composer = track->composer();
    Meta::YearPtr year = track->year();
    Meta::StatisticsPtr statistics = track->statistics();

    if( !track->isPlayable() )
        text += i18n( "<b>Note:</b> This track is not playable.<br>%1", track->notPlayableReason() );

    if( s_tooltipColumns[Playlist::Title] )
        text += HTMLLine( Playlist::Title, track->name() );

    if( s_tooltipColumns[Playlist::Artist] && artist )
        text += HTMLLine( Playlist::Artist, artist->name() );

    // only show albumArtist when different from artist (it should suffice to compare pointers)
    if( s_tooltipColumns[Playlist::AlbumArtist] && albumArtist && albumArtist != artist )
        text += HTMLLine( Playlist::AlbumArtist, albumArtist->name() );

    if( s_tooltipColumns[Playlist::Album] && album )
        text += HTMLLine( Playlist::Album, album->name() );

    if( s_tooltipColumns[Playlist::DiscNumber] )
        text += HTMLLine( Playlist::DiscNumber, track->discNumber() );

    if( s_tooltipColumns[Playlist::TrackNumber] )
        text += HTMLLine( Playlist::TrackNumber, track->trackNumber() );

    if( s_tooltipColumns[Playlist::Composer] && composer )
        text += HTMLLine( Playlist::Composer, composer->name() );

    if( s_tooltipColumns[Playlist::Genre] && genre )
        text += HTMLLine( Playlist::Genre, genre->name() );

    if( s_tooltipColumns[Playlist::Year] && year && year->year() > 0 )
        text += HTMLLine( Playlist::Year, year->year() );

    if( s_tooltipColumns[Playlist::Bpm] )
        text += HTMLLine( Playlist::Bpm, track->bpm() );

    if( s_tooltipColumns[Playlist::Comment]) {
        if ( !(fitsInOneLineHTML( track->comment() ) ) )
            text += HTMLLine( Playlist::Comment, i18n( "(...)" ) );
        else
            text += HTMLLine( Playlist::Comment, track->comment() );
    }

    if( s_tooltipColumns[Playlist::Labels] && !track->labels().empty() )
    {
        QStringList labels;
        foreach( Meta::LabelPtr label, track->labels() )
        {
            if( label )
                labels << label->name();
        }
        text += HTMLLine( Playlist::Labels, labels.join( QStringLiteral(", ") ) );
    }

    if( s_tooltipColumns[Playlist::Score] )
        text += HTMLLine( Playlist::Score, statistics->score() );

    if( s_tooltipColumns[Playlist::Rating] )
        text += HTMLLine( Playlist::Rating, QString::number( statistics->rating()/2.0 ) );

    if( s_tooltipColumns[Playlist::PlayCount] )
        text += HTMLLine( Playlist::PlayCount, statistics->playCount(), true );

    if( s_tooltipColumns[Playlist::LastPlayed] && statistics->lastPlayed().isValid() )
        text += HTMLLine( Playlist::LastPlayed, QLocale().toString( statistics->lastPlayed() ) );

    if( s_tooltipColumns[Playlist::Bitrate] && track->bitrate() )
        text += HTMLLine( Playlist::Bitrate, i18nc( "%1: bitrate", "%1 kbps", track->bitrate() ) );

    if( text.isEmpty() )
        text = i18n( "No extra information available" );
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
        return  m_items.at( row )->track()->has<Capabilities::MultiSourceCapability>();

    else if ( role == StopAfterTrackRole )
        return Actions::instance()->willStopAfterTrack( idAt( row ) );

    else if ( role == Qt::ToolTipRole )
    {
        Meta::TrackPtr track = m_items.at( row )->track();
        if( s_showToolTip )
            return tooltipFor( track );
        else if( !track->isPlayable() )
            return i18n( "<b>Note:</b> This track is not playable.<br>%1", track->notPlayableReason() );
    }

    else if ( role == Qt::DisplayRole )
    {
        Meta::TrackPtr track = m_items.at( row )->track();
        Meta::AlbumPtr album = track->album();
        Meta::StatisticsPtr statistics = track->statistics();
        switch ( index.column() )
        {
            case PlaceHolder:
                break;
            case Album:
            {
                if( album )
                    return album->name();
                break;
            }
            case AlbumArtist:
            {
                if( album )
                {
                    Meta::ArtistPtr artist = album->albumArtist();
                    if( artist )
                        return artist->name();
                }
                break;
            }
            case Artist:
            {
                Meta::ArtistPtr artist = track->artist();
                if( artist )
                    return artist->name();
                break;
            }
            case Bitrate:
            {
                return Meta::prettyBitrate( track->bitrate() );
            }
            case Bpm:
            {
                if( track->bpm() > 0.0 )
                    return QString::number( track->bpm() );
                break;
            }
            case Comment:
            {
                return track->comment();
            }
            case Composer:
            {
                Meta::ComposerPtr composer = track->composer();
                if( composer )
                    return composer->name();
                break;
            }
            case CoverImage:
            {
                if( album )
                    return The::svgHandler()->imageWithBorder( album, 100 ); //FIXME:size?
                break;
            }
            case Directory:
            {
                if( track->playableUrl().isLocalFile() )
                    return track->playableUrl().adjusted(QUrl::RemoveFilename).path();
                break;
            }
            case DiscNumber:
            {
                if( track->discNumber() > 0 )
                    return track->discNumber();
                break;
            }
            case Filename:
            {

                if( track->playableUrl().isLocalFile() )
                    return track->playableUrl().fileName();
                break;
            }
            case Filesize:
            {
                return Meta::prettyFilesize( track->filesize() );
            }
            case Genre:
            {
                Meta::GenrePtr genre = track->genre();
                if( genre )
                    return genre->name();
                break;
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
                if( track )
                {
                    QStringList labelNames;
                    foreach( const Meta::LabelPtr &label, track->labels() )
                    {
                        labelNames << label->prettyName();
                    }
                    return labelNames.join( QStringLiteral(", ") );
                }
                break;
            }
            case LastPlayed:
            {
                if( statistics->playCount() == 0 )
                    return i18nc( "The amount of time since last played", "Never" );
                else if( statistics->lastPlayed().isValid() )
                    return Amarok::verboseTimeSince( statistics->lastPlayed() );
                else
                    return i18nc( "When this track was last played", "Unknown" );
            }
            case Length:
            {
                return Meta::msToPrettyTime( track->length() );
            }
            case LengthInSeconds:
            {
                return track->length() / 1000;
            }
            case Mood:
            {
                return QString(); //FIXME
            }
            case PlayCount:
            {
                return statistics->playCount();
            }
            case Rating:
            {
                return statistics->rating();
            }
            case SampleRate:
            {
                if( track->sampleRate() > 0 )
                    return track->sampleRate();
                break;
            }
            case Score:
            {
                return int( statistics->score() ); // Cast to int, as we don't need to show the decimals in the view..
            }
            case Source:
            {
                QString sourceName;
                Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
                if ( sic )
                {
                    sourceName = sic->sourceName();
                    delete sic;
                }
                else
                {
                    sourceName = track->collection() ? track->collection()->prettyName() : QString();
                }
                return sourceName;
            }
            case SourceEmblem:
            {
                QPixmap emblem;
                Capabilities::SourceInfoCapability *sic = track->create<Capabilities::SourceInfoCapability>();
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
                return track->prettyName();
            }
            case TitleWithTrackNum:
            {
                QString trackString;
                QString trackName = track->prettyName();
                if( track->trackNumber() > 0 )
                {
                    QString trackNumber = QString::number( track->trackNumber() );
                    trackString =  QString( trackNumber + " - " + trackName );
                } else
                    trackString = trackName;

                return trackString;
            }
            case TrackNumber:
            {
                if( track->trackNumber() > 0 )
                    return track->trackNumber();
                break;
            }
            case Type:
            {
                return track->type();
            }
            case Year:
            {
                Meta::YearPtr year = track->year();
                if( year && year->year() > 0 )
                    return year->year();
                break;
            }
            default:
                return QVariant(); // returning a variant instead of a string inside a variant is cheaper

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
    ret << QStringLiteral("text/uri-list"); //we do accept urls
    return ret;
}

QMimeData*
Playlist::Model::mimeData( const QModelIndexList &indexes ) const
{
    AmarokMimeData* mime = new AmarokMimeData();
    Meta::TrackList selectedTracks;

    for( const QModelIndex &it : indexes )
        selectedTracks << m_items.at( it.row() )->track();

    mime->setTracks( selectedTracks );
    return mime;
}

bool
Playlist::Model::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int, const QModelIndex &parent )
{
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
            std::stable_sort( tracks.begin(), tracks.end(), Meta::Track::lessThan );

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
        TrackLoader *dl = new TrackLoader(); // auto-deletes itself
        dl->setProperty( "beginRow", beginRow );
        connect( dl, &TrackLoader::finished, this, &Model::insertTracksFromTrackLoader );
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
        Q_EMIT activeTrackChanged( m_items.at( row )->id() );
    }
    else
    {
        m_activeRow = -1;
        Q_EMIT activeTrackChanged( 0 );
    }
}

void
Playlist::Model::emitQueueChanged()
{
    Q_EMIT queueChanged();
}

int
Playlist::Model::queuePositionOfRow( int row )
{
    return Actions::instance()->queuePosition( idAt( row ) ) + 1;
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
Playlist::Model::containsTrack( const Meta::TrackPtr& track ) const
{
    for( Item* i : m_items )
    {
        if ( *i->track() == *track )
            return true;
    }
    return false;
}

int
Playlist::Model::firstRowForTrack( const Meta::TrackPtr& track ) const
{
    int row = 0;
    for( Item* i : m_items )
    {
        if ( *i->track() == *track )
            return row;
        row++;
    }
    return -1;
}

QSet<int>
Playlist::Model::allRowsForTrack( const Meta::TrackPtr& track ) const
{
    QSet<int> trackRows;

    int row = 0;
    for( Item* i : m_items )
    {
        if ( *i->track() == *track )
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
    Item* item = m_itemIds.value( id, nullptr );
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
    Item* item = m_itemIds.value( id, nullptr );
    if ( item )
        return item->state();
    else
        return Item::Invalid;
}

void
Playlist::Model::metadataChanged(const Meta::TrackPtr &track )
{
    int row = 0;
    for( Item* i : m_items )
    {
        if ( i->track() == track )
        {
            // ensure that we really have the correct album subscribed (in case it changed)
            Meta::AlbumPtr album = track->album();
            if( album )
                subscribeTo( album );

            Q_EMIT dataChanged( index( row, 0 ), index( row, columnCount() - 1 ) );
        }
        row++;
    }
}

void
Playlist::Model::metadataChanged(const Meta::AlbumPtr &album )
{
    // Mainly to get update about changed covers

    // -- search for all the tracks having this album
    bool found = false;
    const int size = m_items.size();
    for ( int i = 0; i < size; i++ )
    {
        if ( m_items.at( i )->track()->album() == album )
        {
            Q_EMIT dataChanged( index( i, 0 ), index( i, columnCount() - 1 ) );
            found = true;
            debug()<<"Metadata updated for album"<<album->prettyName();
        }
    }

    // -- unsubscribe if we don't have a track from that album left.
    // this can happen if the album of a track changed
    if( !found )
        unsubscribeFrom( album );
}

bool
Playlist::Model::exportPlaylist( const QString &path, bool relative )
{
    // check queue state
    QQueue<quint64> queueIds = The::playlistActions()->queue();
    QList<int> queued;
    foreach( quint64 id, queueIds ) {
      queued << rowForId( id );
    }
    return Playlists::exportPlaylistFile( tracks(), QUrl::fromLocalFile(path), relative, queued);
}

Meta::TrackList
Playlist::Model::tracks()
{
    Meta::TrackList tl;
    for( Item* item : m_items )
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
    default:         return QStringLiteral("This is a bug.");
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
        EngineController *engine = The::engineController();
        if( engine ) // test cases might create a playlist without having an EngineController
        {
            const Meta::TrackPtr engineTrack = engine->currentTrack();
            if( engineTrack )
            {
                int engineRow = firstRowForTrack( engineTrack );
                if( engineRow > -1 )
                    setActiveRow( engineRow );
            }
        }
    }
}

static bool
removeCmdLessThanByRow( const Playlist::RemoveCmd &left, const Playlist::RemoveCmd &right )
{
    return left.second < right.second;
}

void
Playlist::Model::removeTracksCommand( const RemoveCmdList &passedCmds )
{
    DEBUG_BLOCK
    if ( passedCmds.size() < 1 )
        return;

    if ( passedCmds.size() == m_items.size() )
    {
        clearCommand();
        return;
    }

    // sort tracks to remove by their row
    RemoveCmdList cmds( passedCmds );
    std::sort( cmds.begin(), cmds.end(), removeCmdLessThanByRow );

    // update the active row
    if( m_activeRow >= 0 )
    {
        int activeShift = 0;
        foreach( const RemoveCmd &rc, cmds )
        {
            if( rc.second < m_activeRow )
                activeShift++;
            else if( rc.second == m_activeRow )
                m_activeRow = -1; // disappeared
            else
                break; // we got over it, nothing left to do
        }
        if( m_activeRow >= 0 ) // not deleted
            m_activeRow -= activeShift;
    }

    QSet<Meta::TrackPtr> trackUnsubscribeCandidates;
    QSet<Meta::AlbumPtr> albumUnsubscribeCandidates;

    QListIterator<RemoveCmd> it( cmds );
    int removedRows = 0;
    while( it.hasNext() )
    {
        int startRow = it.next().second;
        int endRow = startRow;

        // find consecutive runs of rows, this is important to group begin/endRemoveRows(),
        // which are very costly when there are many proxymodels and a view above.
        while( it.hasNext() && it.peekNext().second == endRow + 1 )
        {
            it.next();
            endRow++;
        }

        beginRemoveRows( QModelIndex(), startRow - removedRows, endRow - removedRows );
        for( int row = startRow; row <= endRow; row++ )
        {
            Item *removedItem = m_items.at( row - removedRows );
            m_items.removeAt( row - removedRows );
            m_itemIds.remove( removedItem->id() );

            const Meta::TrackPtr &track = removedItem->track();
            // update totals here so they're right when endRemoveRows() called
            m_totalLength -= track->length();
            m_totalSize -= track->filesize();
            trackUnsubscribeCandidates.insert( track );
            Meta::AlbumPtr album = track->album();
            if( album )
                albumUnsubscribeCandidates.insert( album );

            delete removedItem; // note track is by reference, needs removedItem alive
            removedRows++;
        }
        endRemoveRows();
    }

    // unsubscribe from tracks no longer present in playlist
    foreach( Meta::TrackPtr track, trackUnsubscribeCandidates )
    {
        if( !containsTrack( track ) )
            unsubscribeFrom( track );
    }

    // unsubscribe from albums no longer present im playlist
    QSet<Meta::AlbumPtr> remainingAlbums;
    foreach( const Item *item, m_items )
    {
        Meta::AlbumPtr album = item->track()->album();
        if( album )
            remainingAlbums.insert( album );
    }
    foreach( Meta::AlbumPtr album, albumUnsubscribeCandidates )
    {
        if( !remainingAlbums.contains( album ) )
            unsubscribeFrom( album );
    }

    // make sure that there are enough tracks if we just removed from a dynamic playlist.
    // This call needs to be delayed or else we would mess up the undo queue
    // BUG: 259675
    // FIXME: removing the track and normalizing the playlist should be grouped together
    //        so that an undo operation undos both.
    QTimer::singleShot(0, Playlist::Actions::instance(), &Playlist::Actions::normalizeDynamicPlaylist);
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

    int min = INT_MAX;
    int max = INT_MIN;
    foreach( const MoveCmd &rc, cmds )
    {
        min = qMin( min, rc.first );
        max = qMax( max, rc.first );
    }

    if( min < 0 || max >= m_items.size() )
    {
        error() << "Wrong row numbers given";
        return;
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
    //   - Call 'beginMoveRows()' / 'endMoveRows()'. Drawback: we'd need to do N of them, all causing resorts etc.
    //   - Emit 'layoutAboutToChange' / 'layoutChanged'. Drawback: unspecific, 'changePersistentIndex()' complications.
    //   - Emit 'dataChanged'. Drawback: a bit inappropriate. But not wrong.
    Q_EMIT dataChanged( index( min, 0 ), index( max, columnCount() - 1 ) );

    //update the active row
    m_activeRow = newActiveRow;
}


// When doing a 'setStateOfItem_batch', we Q_EMIT 1 crude 'dataChanged' signal. If we're
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
        Q_EMIT dataChanged( index( m_setStateOfItem_batchMinRow, 0 ), index( m_setStateOfItem_batchMaxRow, columnCount() - 1 ) );

    m_setStateOfItem_batchMinRow = -1;
}

void
Playlist::Model::setStateOfItem( Item *item, int row, Item::State state )
{
    item->setState( state );

    if ( m_setStateOfItem_batchMinRow == -1 )    // If not in batch mode
        Q_EMIT dataChanged( index( row, 0 ), index( row, columnCount() - 1 ) );
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
