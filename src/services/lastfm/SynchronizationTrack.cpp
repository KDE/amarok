/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "SynchronizationTrack.h"

#include "core/support/Debug.h"
#include "core/support/SemaphoreReleaser.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QNetworkReply>

#include <Track.h>
#include <XmlQuery.h>

SynchronizationTrack::SynchronizationTrack( QString artist, QString album, QString name,
                                            int playCount, bool useFancyRatingTags )
    : m_artist( artist )
    , m_album( album )
    , m_name( name )
    , m_rating( 0 )
    , m_newRating( 0 )
    , m_playCount( playCount )
    , m_useFancyRatingTags( useFancyRatingTags )
{
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );

    connect( this, &SynchronizationTrack::startTagAddition,
             this, &SynchronizationTrack::slotStartTagAddition );
    connect( this, &SynchronizationTrack::startTagRemoval,
             this, &SynchronizationTrack::slotStartTagRemoval );
}

QString
SynchronizationTrack::name() const
{
    return m_name;
}

QString
SynchronizationTrack::album() const
{
    return m_album;
}

QString
SynchronizationTrack::artist() const
{
    return m_artist;
}

int
SynchronizationTrack::rating() const
{
    return m_rating;
}

void
SynchronizationTrack::setRating( int rating )
{
    m_newRating = rating;
}

QDateTime
SynchronizationTrack::firstPlayed() const
{
    return QDateTime(); // no support on Last.fm side yet
}

QDateTime
SynchronizationTrack::lastPlayed() const
{
    return QDateTime(); // no support on Last.fm side yet
}

int
SynchronizationTrack::playCount() const
{
    return m_playCount;
}

QSet<QString>
SynchronizationTrack::labels() const
{
    return m_labels;
}

void
SynchronizationTrack::setLabels( const QSet<QString> &labels )
{
    m_newLabels = labels;
}

void
SynchronizationTrack::commit()
{
    if( m_newRating == m_rating && m_newLabels == m_labels )
        return;

    const QSet<QString> existingLabels = m_labels | m_ratingLabels;
    if( m_useFancyRatingTags )
    {
        // implicitly we remove all ratingLabels here by not including them in m_newLabels
        if( m_newRating > 0 )
        {
            QString ratingLabel = QString( "%1 of 10 stars" ).arg( m_newRating );
            m_newLabels.insert( ratingLabel );
            m_ratingLabels = QSet<QString>() << ratingLabel;
        }
    }
    else
        m_newLabels |= m_ratingLabels; // preserve all rating labels

    QSet<QString> toAdd = m_newLabels - existingLabels;
    QSet<QString> toRemove = existingLabels - m_newLabels;

    // first remove, than add Last.fm may limit number of track tags
    if( !toRemove.isEmpty() )
    {
        Q_ASSERT( m_semaphore.available() == 0 );
        m_tagsToRemove = toRemove.values();
        Q_EMIT startTagRemoval();
        m_semaphore.acquire(); // wait for the job to complete
        m_tagsToRemove.clear();
    }
    if( !toAdd.isEmpty() )
    {
        Q_ASSERT( m_semaphore.available() == 0 );
        Q_EMIT startTagAddition( toAdd.values() );
        m_semaphore.acquire(); // wait for the job to complete
    }

    m_rating = m_newRating;
    m_labels = m_newLabels - m_ratingLabels;
}

void
SynchronizationTrack::parseAndSaveLastFmTags( const QSet<QString> &tags )
{
    m_labels.clear();
    m_ratingLabels.clear();
    m_rating = 0;

    // we still match and explicitly ignore rating tags even in m_useFancyRatingTags is false
    QRegularExpression rx( QRegularExpression::anchoredPattern( "([0-9]{1,3}) of ([0-9]{1,3}) stars" ), QRegularExpression::CaseInsensitiveOption );
    for( const QString &tag : tags )
    {
        QRegularExpressionMatch rmatch = rx.match( tag );
        if( rmatch.hasMatch() )
        {
            m_ratingLabels.insert( tag );
            QStringList texts = rmatch.capturedTexts();
            if( texts.count() != 3 )
                continue;
            qreal numerator = texts.at( 1 ).toDouble();
            qreal denominator = texts.at( 2 ).toDouble();
            if( denominator == 0.0 )
                continue;
            m_rating = qBound( 0, qRound( 10.0 * numerator / denominator ), 10 );
        }
        else
            m_labels.insert( tag );
    }
    if( !m_useFancyRatingTags || m_ratingLabels.count() > 1 )
        m_rating = 0; // ambiguous or not requested

    m_newLabels = m_labels;
    m_newRating = m_rating;
}

void
SynchronizationTrack::slotStartTagAddition( QStringList tags )
{
    lastfm::MutableTrack track;
    track.setArtist( m_artist );
    track.setAlbum( m_album );
    track.setTitle( m_name );

    if( tags.count() > 10 )
        tags = tags.mid( 0, 10 ); // Last.fm says 10 tags is max
    QNetworkReply *reply = track.addTags( tags );
    connect( reply, &QNetworkReply::finished, this, &SynchronizationTrack::slotTagsAdded );
}

void
SynchronizationTrack::slotStartTagRemoval()
{
    Q_ASSERT( m_tagsToRemove.count() );
    lastfm::MutableTrack track;
    track.setArtist( m_artist );
    track.setAlbum( m_album );
    track.setTitle( m_name );

    QNetworkReply *reply = track.removeTag( m_tagsToRemove.takeFirst() );
    connect( reply, &QNetworkReply::finished, this, &SynchronizationTrack::slotTagRemoved );
}

void
SynchronizationTrack::slotTagsAdded()
{
    SemaphoreReleaser releaser( &m_semaphore );
    QNetworkReply *reply =  qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot cast sender to QNetworkReply. (?)";
        return;
    }
    reply->deleteLater();

    lastfm::XmlQuery lfm;
    if( !lfm.parse( reply->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "error adding tags:" << lfm.parseError().message();
        return;
    }
}

void
SynchronizationTrack::slotTagRemoved()
{
    SemaphoreReleaser releaser( &m_semaphore );
    QNetworkReply *reply =  qobject_cast<QNetworkReply *>( sender() );
    if( !reply )
    {
        warning() << __PRETTY_FUNCTION__ << "cannot cast sender to QNetworkReply. (?)";
        return;
    }
    reply->deleteLater();

    lastfm::XmlQuery lfm;
    if( !lfm.parse( reply->readAll() ) )
    {
        warning() << __PRETTY_FUNCTION__ << "error removing a tag:" << lfm.parseError().message();
        return;
    }

    // remove the next one, sadly only one at a time can be removed
    if( !m_tagsToRemove.isEmpty() )
    {
        releaser.dontRelease();
        Q_EMIT startTagRemoval();
    }
}
