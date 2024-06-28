/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include "core/meta/Meta.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Observer.h"
#include "core/meta/Statistics.h"
#include "core/meta/TrackEditor.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QImage>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QNetworkConfigurationManager>
#endif

#include <KLocalizedString>

using namespace Meta;

//Meta::Track

QString
Meta::Track::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return prettyUrl();
}

bool
Meta::Track::inCollection() const
{
    return false;
}

Collections::Collection*
Meta::Track::collection() const
{
    return nullptr;
}

Meta::LabelList
Meta::Track::labels() const
{
    return Meta::LabelList();
}

QString
Meta::Track::cachedLyrics() const
{
    return QString();
}

void
Meta::Track::setCachedLyrics( const QString &lyrics )
{
    Q_UNUSED( lyrics )
}

void
Meta::Track::addLabel( const QString &label )
{
    Q_UNUSED( label )
}

void
Meta::Track::addLabel( const Meta::LabelPtr &label )
{
    Q_UNUSED( label )
}

void
Meta::Track::removeLabel( const Meta::LabelPtr &label )
{
    Q_UNUSED( label )
}

QDateTime
Meta::Track::createDate() const
{
    return QDateTime();
}

QDateTime
Meta::Track::modifyDate() const
{
    return QDateTime();
}

qreal
Meta::Track::replayGain( Meta::ReplayGainTag mode ) const
{
    Q_UNUSED( mode )
    return 0.0;
}

void
Meta::Track::prepareToPlay()
{
}

void
Meta::Track::finishedPlaying( double playedFraction )
{
    qint64 len = length();
    bool updatePlayCount;
    if( len <= 30 * 1000 )
        updatePlayCount = ( playedFraction >= 1.0 );
    else
        // at least half the song or at least 5 minutes played
        updatePlayCount = ( playedFraction >= 0.5 || ( playedFraction * len ) >= 5 * 60 * 1000 );

    StatisticsPtr stats = statistics();
    stats->beginUpdate();
    // we should update score even if updatePlayCount is false to record skips
    stats->setScore( Amarok::computeScore( stats->score(), stats->playCount(), playedFraction ) );
    if( updatePlayCount )
    {
        stats->setPlayCount( stats->playCount() + 1 );
        if( !stats->firstPlayed().isValid() )
            stats->setFirstPlayed( QDateTime::currentDateTime() );
        stats->setLastPlayed( QDateTime::currentDateTime() );
    }
    stats->endUpdate();
}

void
Meta::Track::notifyObservers() const
{
    notifyObserversHelper<Track, Observer>( this );
}

bool
Meta::Track::operator==( const Meta::Track &track ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &track );
}

bool
Meta::Track::lessThan( const Meta::TrackPtr& left, const Meta::TrackPtr& right )
{
    if( !left || !right ) // These should never be 0, but it can apparently happen (https://bugs.kde.org/show_bug.cgi?id=181187)
        return false;

    if( left->album() && right->album() )
        if( left->album()->name() == right->album()->name() )
        {
            if( left->discNumber() < right->discNumber() )
                return true;
            else if( left->discNumber() > right->discNumber() )
                return false;

            if( left->trackNumber() < right->trackNumber() )
                return true;
            if( left->trackNumber() > right->trackNumber() )
                return false;
        }

    if( left->artist() && right->artist() )
    {
        int compare = QString::localeAwareCompare( left->artist()->prettyName(), right->artist()->prettyName() );
        if ( compare < 0 )
            return true;
        else if ( compare > 0 )
            return false;
    }

    if( left->album() && right->album() )
    {
        int compare = QString::localeAwareCompare( left->album()->prettyName(), right->album()->prettyName() );
        if ( compare < 0 )
            return true;
        else if ( compare > 0 )
            return false;
    }

    return QString::localeAwareCompare( left->prettyName(), right->prettyName() ) < 0;
}

bool
Track::isPlayable() const
{
    return notPlayableReason().isEmpty();
}

QString
Track::networkNotPlayableReason() const
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QNetworkConfigurationManager mgr;
    if( !mgr.isOnline() )
        return i18n( "No network connection" );
#endif

    return QString();
}

QString
Track::localFileNotPlayableReason( const QString &path ) const
{
    QFileInfo trackFileInfo = QFileInfo( path );
    if( !trackFileInfo.exists() )
        return i18n( "File does not exist" );
    if( !trackFileInfo.isFile() )
        return i18n( "Not a file" );
    if( !trackFileInfo.isReadable() )
        return i18n( "No read permissions" );
    return QString();
}

TrackEditorPtr
Track::editor()
{
    return TrackEditorPtr();
}

StatisticsPtr
Track::statistics()
{
    // return dummy implementation
    return StatisticsPtr( new Statistics() );
}

ConstStatisticsPtr
Track::statistics() const
{
    StatisticsPtr statistics = const_cast<Track *>( this )->statistics();
    return ConstStatisticsPtr( statistics.data() );
}


//Meta::Artist

QString
Meta::Artist::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return i18n("Unknown Artist");
}

void
Meta::Artist::notifyObservers() const
{
    m_sortableName.clear(); // name() may have changed, recompute sortableName next time
    notifyObserversHelper<Artist, Observer>( this );
}

bool
Meta::Artist::operator==( const Meta::Artist &artist ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &artist );
}

QString
Meta::Artist::sortableName() const
{
    if( !m_sortableName.isEmpty() )
        return m_sortableName;

    const QString &n = name();
    if( n.startsWith( QLatin1String("the "), Qt::CaseInsensitive ) )
    {
        QStringView article = n.leftRef( 3 );
        QStringView subject = n.midRef( 4 );
        m_sortableName = QStringLiteral( "%1, %2" ).arg( subject.toString(), article.toString() );
    }
    else if( n.startsWith( QLatin1String("dj "), Qt::CaseInsensitive ) )
    {
        QStringView article = n.leftRef( 2 );
        QStringView subject = n.midRef( 3 );
        m_sortableName = QStringLiteral( "%1, %2" ).arg( subject.toString(), article.toString() );
    }
    else
        m_sortableName = n;
    return m_sortableName;
}

//Meta::Album

QString
Meta::Album::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return i18n("Unknown Album");
}

void
Meta::Album::notifyObservers() const
{
    notifyObserversHelper<Album, Observer>( this );
}

/*
 * This is the base class's image() function, which returns just an null image.
 * Retrieval of the cover for the actual album is done by subclasses.
 */
QImage
Meta::Album::image( int size ) const
{
    Q_UNUSED( size );
    return QImage();
}

bool
Meta::Album::operator==( const Meta::Album &album ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &album );
}

//Meta::Genre

QString
Meta::Genre::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return i18n("Unknown Genre");
}

void
Meta::Genre::notifyObservers() const
{
    notifyObserversHelper<Genre, Observer>( this );
}

bool
Meta::Genre::operator==( const Meta::Genre &genre ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const void*>( &genre );
}

//Meta::Composer

QString
Meta::Composer::prettyName() const
{
    if( !name().isEmpty() )
        return name();
    return i18n("Unknown Composer");
}

void
Meta::Composer::notifyObservers() const
{
    notifyObserversHelper<Composer, Observer>( this );
}

bool
Meta::Composer::operator==( const Meta::Composer &composer ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &composer );
}

//Meta::Year

void
Meta::Year::notifyObservers() const
{
    notifyObserversHelper<Year, Observer>( this );
}

bool
Meta::Year::operator==( const Meta::Year &year ) const
{
    return dynamic_cast<const void*>( this ) == dynamic_cast<const  void*>( &year );
}
