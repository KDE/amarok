/****************************************************************************************
 * Copyright (c) 2008 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MemoryFilter.h"

#include <QDateTime>

namespace FilterFactory
{
    MemoryFilter* filter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        MemoryFilter *result = 0;
        switch( value )
        {
            case Meta::valTitle:
            {
                result = new TitleMemoryFilter( filter, matchBegin, matchEnd );
                break;
            }
            case Meta::valAlbum:
            {
                result = new AlbumMemoryFilter( filter, matchBegin, matchEnd );
                break;
            }
            case Meta::valArtist:
            {
                result = new ArtistMemoryFilter( filter, matchBegin, matchEnd );
                break;
            }
            case Meta::valYear:
            {
                result = new YearMemoryFilter( filter, matchBegin, matchEnd );
                break;
            }
            case Meta::valComposer:
            {
                result = new ComposerMemoryFilter( filter, matchBegin, matchEnd );
                break;
            }
            case Meta::valComment:
            {
                result = new CommentMemoryFilter( filter, matchBegin, matchEnd );
            }
        }
        return result;
    }

    MemoryFilter* numberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare )
    {
        NumberMemoryFilter *result = 0;
        switch( value )
        {
            case Meta::valTrackNr:
                result = new TrackNumberFilter();
                break;
            case Meta::valDiscNr:
                result = new DiscNumberFilter();
                break;
            case Meta::valRating:
                result = new RatingFilter();
                break;
            case Meta::valScore:
                result = new ScoreFilter();
                break;
            case Meta::valPlaycount:
                result = new PlaycountFilter();
                break;
            case Meta::valFirstPlayed:
                result = new FirstPlayedFilter();
                break;
            case Meta::valLastPlayed:
                result = new LastPlayedFilter();
                break;
            case Meta::valLength:
                result = new LengthFilter();
                break;
            case Meta::valFilesize:
                result = new FilesizeFilter();
                break;
            case Meta::valSamplerate:
                result = new SampleRateFilter();
                break;
            case Meta::valBitrate:
                result = new BitrateFilter();
                break;
            case Meta::valCreateDate:
                result = new CreateDateFilter();
                break;
        }
        Q_ASSERT_X( result, "FilterFactory::numberFilter", "called numberFilter with an illegal value, value was " + value );
        if (result)
            result->setFilter( filter, compare );

        return result;
    }
}

MemoryFilter::MemoryFilter()
{
}

MemoryFilter::~MemoryFilter()
{
}

ContainerMemoryFilter::ContainerMemoryFilter()
    : MemoryFilter()
{
}

ContainerMemoryFilter::~ContainerMemoryFilter()
{
    qDeleteAll( m_filters );
}

void
ContainerMemoryFilter::addFilter( MemoryFilter *filter )
{
    m_filters.append( filter );
}

AndContainerMemoryFilter::AndContainerMemoryFilter()
    : ContainerMemoryFilter()
{
}

AndContainerMemoryFilter::~AndContainerMemoryFilter()
{
}

bool
AndContainerMemoryFilter::filterMatches( const Meta::TrackPtr &track ) const
{
    if( m_filters.isEmpty() )
        return false;

    foreach( MemoryFilter *filter, m_filters )
    {
        if( !filter->filterMatches( track ) )
            return false;
    }
    return true;
}

OrContainerMemoryFilter::OrContainerMemoryFilter()
    : ContainerMemoryFilter()
{
}

OrContainerMemoryFilter::~OrContainerMemoryFilter()
{
}

bool
OrContainerMemoryFilter::filterMatches( const Meta::TrackPtr &track ) const
{
    if( m_filters.isEmpty() )
        return false;

    foreach( MemoryFilter *filter, m_filters )
    {
        if( filter && filter->filterMatches( track ) )
            return true;
    }
    return false;
}

NegateMemoryFilter::NegateMemoryFilter( MemoryFilter *filter )
    :MemoryFilter()
    , m_filter( filter )
{
}

NegateMemoryFilter::~NegateMemoryFilter()
{
    delete m_filter;
}

bool
NegateMemoryFilter::filterMatches( const Meta::TrackPtr &track ) const
{
    return !m_filter->filterMatches( track );
}

StringMemoryFilter::StringMemoryFilter()
    : MemoryFilter()
    , m_matchBegin( false )
    , m_matchEnd( false )
{
}

StringMemoryFilter::~StringMemoryFilter()
{
}

void
StringMemoryFilter::setFilter( const QString &filter, bool matchBegin, bool matchEnd )
{
    m_filter = filter;
    m_matchBegin = matchBegin;
    m_matchEnd = matchEnd;
}

bool
StringMemoryFilter::filterMatches( const Meta::TrackPtr &track ) const
{
    const QString &str = value( track );
    if( m_matchBegin && m_matchEnd )
    {
        return QString::compare( str, m_filter, Qt::CaseInsensitive ) == 0;
    }
    else if( m_matchBegin )
    {
        return str.startsWith( m_filter, Qt::CaseInsensitive );
    }
    else if( m_matchEnd )
    {
        return str.endsWith( m_filter, Qt::CaseInsensitive );
    }
    else
    {
        return str.contains( m_filter, Qt::CaseInsensitive );
    }
}

TitleMemoryFilter::TitleMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

TitleMemoryFilter::~TitleMemoryFilter()
{
}

QString
TitleMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->name();
}


ArtistMemoryFilter::ArtistMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

ArtistMemoryFilter::~ArtistMemoryFilter()
{
}

QString
ArtistMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->artist()->name();
}

AlbumMemoryFilter::AlbumMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

AlbumMemoryFilter::~AlbumMemoryFilter()
{
}

QString
AlbumMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->album()->name();
}

GenreMemoryFilter::GenreMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

GenreMemoryFilter::~GenreMemoryFilter()
{
}

QString
GenreMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->genre()->name();
}

ComposerMemoryFilter::ComposerMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

ComposerMemoryFilter::~ComposerMemoryFilter()
{
}

QString
ComposerMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->composer()->name();
}

YearMemoryFilter::YearMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

YearMemoryFilter::~YearMemoryFilter()
{
}

QString
YearMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->year()->name();
}

CommentMemoryFilter::CommentMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : StringMemoryFilter()
{
    setFilter( filter, matchBegin, matchEnd );
}

CommentMemoryFilter::~CommentMemoryFilter()
{
}

QString
CommentMemoryFilter::value( const Meta::TrackPtr &track ) const
{
    return track->comment();
}

NumberMemoryFilter::NumberMemoryFilter()
    : MemoryFilter()
    , m_filter( 0 )
    , m_compare( QueryMaker::Equals )
{
}

NumberMemoryFilter::~NumberMemoryFilter()
{
}

void
NumberMemoryFilter::setFilter( qint64 filter, QueryMaker::NumberComparison compare )
{
    m_filter = filter;
    m_compare = compare;
}

bool
NumberMemoryFilter::filterMatches( const Meta::TrackPtr &track ) const
{
    qint64 currentValue = value( track );
    switch( m_compare )
    {
        case QueryMaker::Equals:
            return currentValue == m_filter;
        case QueryMaker::GreaterThan:
            return currentValue > m_filter;
        case QueryMaker::LessThan:
            return currentValue < m_filter;
    }
    return false;
}

TrackNumberFilter::TrackNumberFilter()
    : NumberMemoryFilter()
{
}

TrackNumberFilter::~TrackNumberFilter()
{
}

qint64
TrackNumberFilter::value( const Meta::TrackPtr &track ) const
{
    return track->trackNumber();
}

DiscNumberFilter::DiscNumberFilter()
    : NumberMemoryFilter()
{
}

DiscNumberFilter::~DiscNumberFilter()
{
}

qint64
DiscNumberFilter::value( const Meta::TrackPtr &track ) const
{
    return track->discNumber();
}

RatingFilter::RatingFilter()
    : NumberMemoryFilter()
{
}

RatingFilter::~RatingFilter()
{
}

qint64
RatingFilter::value( const Meta::TrackPtr &track ) const
{
    return track->rating();
}

ScoreFilter::ScoreFilter()
    : NumberMemoryFilter()
{
}

ScoreFilter::~ScoreFilter()
{
}

qint64
ScoreFilter::value( const Meta::TrackPtr &track ) const
{
    return track->score();
}

PlaycountFilter::PlaycountFilter()
    : NumberMemoryFilter()
{
}

PlaycountFilter::~PlaycountFilter()
{
}

qint64
PlaycountFilter::value( const Meta::TrackPtr &track ) const
{
    return track->playCount();
}

FirstPlayedFilter::FirstPlayedFilter()
    : NumberMemoryFilter()
{
}

FirstPlayedFilter::~FirstPlayedFilter()
{
}

qint64
FirstPlayedFilter::value( const Meta::TrackPtr &track ) const
{
    return track->firstPlayed();
}

LastPlayedFilter::LastPlayedFilter()
    : NumberMemoryFilter()
{
}

LastPlayedFilter::~LastPlayedFilter()
{
}

qint64
LastPlayedFilter::value( const Meta::TrackPtr &track ) const
{
    return track->lastPlayed();
}

LengthFilter::LengthFilter()
    : NumberMemoryFilter()
{
}

LengthFilter::~LengthFilter()
{
}

qint64
LengthFilter::value( const Meta::TrackPtr &track ) const
{
    return track->length();
}

FilesizeFilter::FilesizeFilter()
    : NumberMemoryFilter()
{
}

FilesizeFilter::~FilesizeFilter()
{
}

qint64
FilesizeFilter::value( const Meta::TrackPtr &track ) const
{
    return track->filesize();
}

SampleRateFilter::SampleRateFilter()
    : NumberMemoryFilter()
{
}

SampleRateFilter::~SampleRateFilter()
{
}

qint64
SampleRateFilter::value( const Meta::TrackPtr &track ) const
{
    return track->sampleRate();
}

BitrateFilter::BitrateFilter()
    : NumberMemoryFilter()
{
}

BitrateFilter::~BitrateFilter()
{
}

qint64
BitrateFilter::value( const Meta::TrackPtr &track ) const
{
    return track->bitrate();
}

CreateDateFilter::CreateDateFilter()
    : NumberMemoryFilter()
{
}

CreateDateFilter::~CreateDateFilter()
{
}

qint64
CreateDateFilter::value( const Meta::TrackPtr &track ) const
{
    return track->createDate().toTime_t();
}


