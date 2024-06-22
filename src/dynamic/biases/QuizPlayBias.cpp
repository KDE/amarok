/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "QuizPlayBias"

#include "QuizPlayBias.h"

#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "dynamic/TrackSet.h"

#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>

#include <QTimer>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <KLocalizedString>

QString
Dynamic::QuizPlayBiasFactory::i18nName() const
{ return i18nc("Name of the \"QuizPlay\" bias", "Quiz play"); }

QString
Dynamic::QuizPlayBiasFactory::name() const
{ return Dynamic::QuizPlayBias::sName(); }

QString
Dynamic::QuizPlayBiasFactory::i18nDescription() const
{ return i18nc("Description of the \"QuizPlay\" bias",
               "The \"QuizPlay\" bias adds tracks that start\n"
               "with a character the last track ended with."); }

Dynamic::BiasPtr
Dynamic::QuizPlayBiasFactory::createBias()
{ return Dynamic::BiasPtr( new Dynamic::QuizPlayBias() ); }


// Note: this whole bias does not work correctly for right-to-left languages.


Dynamic::QuizPlayBias::QuizPlayBias()
    : SimpleMatchBias()
    , m_follow( TitleToTitle )
{ }

void
Dynamic::QuizPlayBias::fromXml( QXmlStreamReader *reader )
{
    while (!reader->atEnd()) {
        reader->readNext();

        if( reader->isStartElement() )
        {
            QStringRef name = reader->name();
            if( name == "follow" )
                m_follow = followForName( reader->readElementText(QXmlStreamReader::SkipChildElements) );
            else
            {
                debug()<<"Unexpected xml start element"<<reader->name()<<"in input";
                reader->skipCurrentElement();
            }
        }
        else if( reader->isEndElement() )
        {
            break;
        }
    }
}

void
Dynamic::QuizPlayBias::toXml( QXmlStreamWriter *writer ) const
{
    writer->writeTextElement( QStringLiteral("follow"), nameForFollow( m_follow ) );
}

QString
Dynamic::QuizPlayBias::sName()
{
    return QStringLiteral( "quizPlayBias" );
}

QString
Dynamic::QuizPlayBias::name() const
{
    return Dynamic::QuizPlayBias::sName();
}

QString
Dynamic::QuizPlayBias::toString() const
{
    switch( m_follow )
    {
    case TitleToTitle:
        return i18nc("QuizPlay bias representation",
                     "Tracks whose title start with a\n"
                     "character the last track ended with");
    case ArtistToArtist:
        return i18nc("QuizPlay bias representation",
                     "Tracks whose artist name start\n"
                     "with a character the last track ended with");
    case AlbumToAlbum:
        return i18nc("QuizPlay bias representation",
                     "Tracks whose album name start\n"
                     "with a character the last track ended with");
    }
    return QString();
}

QWidget*
Dynamic::QuizPlayBias::widget( QWidget* parent )
{
    QWidget *widget = new QWidget( parent );
    QVBoxLayout *layout = new QVBoxLayout( widget );

    QLabel *label = new QLabel( i18n( "Last character of the previous song is\n"
                                      "the first character of the next song" ) );
    layout->addWidget( label );

    QComboBox *combo = new QComboBox();
    combo->addItem( i18n( "of the track title (Title quiz)" ),
                    nameForFollow( TitleToTitle ) );
    combo->addItem( i18n( "of the artist (Artist quiz)" ),
                    nameForFollow( ArtistToArtist ) );
    combo->addItem( i18n( "of the album name (Album quiz)" ),
                    nameForFollow( AlbumToAlbum ) );
    switch( m_follow )
    {
    case TitleToTitle:   combo->setCurrentIndex(0); break;
    case ArtistToArtist: combo->setCurrentIndex(1); break;
    case AlbumToAlbum:   combo->setCurrentIndex(2); break;
    }
    connect( combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
             this, &QuizPlayBias::selectionChanged );
    layout->addWidget( combo );

    return widget;
}

Dynamic::TrackSet
Dynamic::QuizPlayBias::matchingTracks( const Meta::TrackList& playlist,
                                       int contextCount, int finalCount,
                                       const Dynamic::TrackCollectionPtr &universe ) const
{
    Q_UNUSED( contextCount );
    Q_UNUSED( finalCount );

    if( playlist.isEmpty() )
        return Dynamic::TrackSet( universe, true );

    // determine the last character we need to quiz
    Meta::TrackPtr lastTrack = playlist.last();
    Meta::DataPtr lastData;
    if( m_follow == TitleToTitle )
        lastData = Meta::DataPtr::staticCast<Meta::Track>(lastTrack);
    else if( m_follow == ArtistToArtist )
        lastData = Meta::DataPtr::staticCast<Meta::Artist>(lastTrack->artist());
    else if( m_follow == AlbumToAlbum )
        lastData = Meta::DataPtr::staticCast<Meta::Album>(lastTrack->album());

    if( !lastData || lastData->name().isEmpty() )
    {
        // debug() << "QuizPlay: no data for"<<lastTrack->name();
        return Dynamic::TrackSet( universe, true );
    }

    m_currentCharacter = lastChar(lastData->name()).toLower();
    // debug() << "QuizPlay: data for"<<lastTrack->name()<<"is"<<m_currentCharacter;

    // -- look if we already buffered it
    if( m_characterTrackMap.contains( m_currentCharacter ) )
        return m_characterTrackMap.value( m_currentCharacter );

    // -- start a new query
    m_tracks = Dynamic::TrackSet( universe, false );
    QTimer::singleShot(0,
                       const_cast<QuizPlayBias*>(this),
                       &QuizPlayBias::newQuery); // create the new query from my parent thread

    return Dynamic::TrackSet();
}

bool
Dynamic::QuizPlayBias::trackMatches( int position,
                                     const Meta::TrackList& playlist,
                                     int contextCount ) const
{
    Q_UNUSED( contextCount );

    if( position <= 0 || position >= playlist.count())
        return true;

    // -- determine the last character we need to quiz
    Meta::TrackPtr lastTrack = playlist[position-1];
    Meta::DataPtr lastData;
    if( m_follow == TitleToTitle )
        lastData = Meta::DataPtr::staticCast<Meta::Track>(lastTrack);
    else if( m_follow == ArtistToArtist )
        lastData = Meta::DataPtr::staticCast<Meta::Artist>(lastTrack->artist());
    else if( m_follow == AlbumToAlbum )
        lastData = Meta::DataPtr::staticCast<Meta::Album>(lastTrack->album());

    if( !lastData || lastData->name().isEmpty() )
        return true;

    // -- determine the first character
    Meta::TrackPtr track = playlist[position];
    Meta::DataPtr data;
    if( m_follow == TitleToTitle )
        data = Meta::DataPtr::staticCast<Meta::Track>(track);
    else if( m_follow == ArtistToArtist )
        data = Meta::DataPtr::staticCast<Meta::Artist>(track->artist());
    else if( m_follow == AlbumToAlbum )
        data = Meta::DataPtr::staticCast<Meta::Album>(track->album());

    if( !data || data->name().isEmpty() )
        return false;

    // -- now compare
    QString lastName = lastData->name();
    QString name = data->name();
    return lastChar( lastName ).toLower() == name[0].toLower();
}


Dynamic::QuizPlayBias::FollowType
Dynamic::QuizPlayBias::follow() const
{
    return m_follow;
}

void
Dynamic::QuizPlayBias::setFollow( Dynamic::QuizPlayBias::FollowType value )
{
    m_follow = value;
    invalidate();
    Q_EMIT changed( BiasPtr(this) );
}

void
Dynamic::QuizPlayBias::updateFinished()
{
    m_characterTrackMap.insert( m_currentCharacter, m_tracks );
    SimpleMatchBias::updateFinished();
}

void
Dynamic::QuizPlayBias::invalidate()
{
    m_characterTrackMap.clear();
    SimpleMatchBias::invalidate();
}


void
Dynamic::QuizPlayBias::selectionChanged( int which )
{
    if( QComboBox *box = qobject_cast<QComboBox*>(sender()) )
        setFollow( followForName( box->itemData( which ).toString() ) );
}

void
Dynamic::QuizPlayBias::newQuery()
{
    // ok, I need a new query maker
    m_qm.reset( CollectionManager::instance()->queryMaker() );

    uint field = 0;
    switch( m_follow )
    {
    case Dynamic::QuizPlayBias::TitleToTitle:   field = Meta::valTitle; break;
    case Dynamic::QuizPlayBias::ArtistToArtist: field = Meta::valArtist; break;
    case Dynamic::QuizPlayBias::AlbumToAlbum:   field = Meta::valAlbum; break;
    }
    m_qm->addFilter( field,  QString(m_currentCharacter), true, false );

    m_qm->setQueryType( Collections::QueryMaker::Custom );
    m_qm->addReturnValue( Meta::valUniqueId );

    connect( m_qm.data(), &Collections::QueryMaker::newResultReady,
             this, &QuizPlayBias::updateReady );
    connect( m_qm.data(), &Collections::QueryMaker::queryDone,
             this, &QuizPlayBias::updateFinished );
    m_qm.data()->run();
}


QChar
Dynamic::QuizPlayBias::lastChar( const QString &str )
{
    int endIndex = str.length();
    int index;

    index = str.indexOf( QLatin1Char('[') );
    if( index > 0 && index < endIndex )
        endIndex = index;

    index = str.indexOf( QLatin1Char('(') );
    if( index > 0 && index < endIndex )
        endIndex = index;

    index = str.indexOf( QLatin1String(" cd"), Qt::CaseInsensitive );
    if( index > 0 && index < endIndex )
        endIndex = index;

    while( endIndex > 0 &&
           (str[ endIndex - 1 ].isSpace() || str[ endIndex - 1 ].isPunct()) )
        endIndex--;

    if( endIndex > 0 )
        return str[ endIndex - 1 ];

    return QChar();
}


QString
Dynamic::QuizPlayBias::nameForFollow( Dynamic::QuizPlayBias::FollowType match )
{
    switch( match )
    {
    case Dynamic::QuizPlayBias::TitleToTitle:   return QStringLiteral("titleQuiz");
    case Dynamic::QuizPlayBias::ArtistToArtist: return QStringLiteral("artistQuiz");
    case Dynamic::QuizPlayBias::AlbumToAlbum:   return QStringLiteral("albumQuiz");
    }
    return QString();
}

Dynamic::QuizPlayBias::FollowType
Dynamic::QuizPlayBias::followForName( const QString &name )
{
    if( name == QLatin1String("titleQuiz") )       return TitleToTitle;
    else if( name == QLatin1String("artistQuiz") ) return ArtistToArtist;
    else if( name == QLatin1String("albumQuiz") )  return AlbumToAlbum;
    else return TitleToTitle;
}




