/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "MemoryCustomValue.h"

#include <QSet>

class SumReturnFunction : public CustomReturnFunction
{
    public:
        SumReturnFunction( CustomReturnValue * rv ) : returnValue( rv ) {}
        ~SumReturnFunction() { delete returnValue; }

        QString value( const Meta::TrackList &tracks ) const
        {
            if( !returnValue )
                return QString::number( 0 );

            double sum = 0.0;
            foreach( const Meta::TrackPtr &track, tracks )
                sum += returnValue->value( track ).toDouble();

            return QString::number( sum );
        }

    private:
        CustomReturnValue *returnValue;
};

class MinimumReturnFunction : public CustomReturnFunction
{
    public:
        MinimumReturnFunction( CustomReturnValue * rv ) : returnValue( rv ) {}
        ~MinimumReturnFunction() { delete returnValue; }

        QString value( const Meta::TrackList &tracks ) const
        {
            if( tracks.empty() || !returnValue )
                return QString::number( 0 );

            double min = returnValue->value( tracks.first() ).toDouble();
            foreach( const Meta::TrackPtr &track, tracks )
                min = qMin( min, returnValue->value( track ).toDouble() );

            return QString::number( min );
        }

    private:
        CustomReturnValue *returnValue;
};

class MaximumReturnFunction : public CustomReturnFunction
{
    public:
        MaximumReturnFunction( CustomReturnValue * rv ) : returnValue( rv ) {}
        ~MaximumReturnFunction() { delete returnValue; }

        QString value( const Meta::TrackList &tracks ) const
        {
            if( tracks.empty() || !returnValue )
                return QString::number( 0 );

            double max = returnValue->value( tracks.first() ).toDouble();
            foreach( const Meta::TrackPtr &track, tracks )
                max = qMax( max, returnValue->value( track ).toDouble() );

            return QString::number( max );
        }

    private:
        CustomReturnValue *returnValue;
};

CustomReturnFunction*
CustomValueFactory::returnFunction( Collections::QueryMaker::ReturnFunction function, qint64 value )
{
    switch( function )
    {
        case Collections::QueryMaker::Count:
        {
            switch( value )
            {
                case Meta::valUrl:
                case Meta::valTitle:
                {
                    return new TrackCounter();
                }
                case Meta::valArtist:
                {
                    return new ArtistCounter();
                }
                case Meta::valAlbum:
                {
                    return new AlbumCounter();
                }
                case Meta::valGenre:
                {
                    return new GenreCounter();
                }
                case Meta::valComposer:
                {
                    return new ComposerCounter();
                }
                case Meta::valYear:
                {
                    return new YearCounter();
                }
                default:
                    return 0;
            }
        }
        case Collections::QueryMaker::Sum:
        {
            CustomReturnValue *crv = CustomValueFactory::returnValue( value );
            return crv ? new SumReturnFunction( crv ) : 0;
        }
        case Collections::QueryMaker::Min:
        {
            CustomReturnValue *crv = CustomValueFactory::returnValue( value );
            return crv ? new MinimumReturnFunction( crv ) : 0;
        }
        case Collections::QueryMaker::Max:
        {
            CustomReturnValue *crv = CustomValueFactory::returnValue( value );
            return crv ? new MaximumReturnFunction( crv ) : 0;
        }
        default:
            return 0;
    }
}

CustomReturnFunction::CustomReturnFunction()
{
}

CustomReturnFunction::~CustomReturnFunction()
{
}

CustomReturnValue::CustomReturnValue()
{
}

CustomReturnValue::~CustomReturnValue()
{
}

TrackCounter::TrackCounter()
{
}

TrackCounter::~TrackCounter()
{
}

QString
TrackCounter::value( const Meta::TrackList &tracks ) const
{
    return QString::number( tracks.count() );
}

ArtistCounter::ArtistCounter()
{
}

ArtistCounter::~ArtistCounter()
{
}

QString
ArtistCounter::value( const Meta::TrackList &tracks ) const
{
    QSet<Meta::ArtistPtr> artists;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        if( track->artist() )
            artists.insert( track->artist() );
    }
    return QString::number( artists.count() );
}

GenreCounter::GenreCounter()
{
}

GenreCounter::~GenreCounter()
{
}

QString
GenreCounter::value( const Meta::TrackList &tracks ) const
{
    QSet<Meta::GenrePtr> genres;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        if( track->genre() )
            genres.insert( track->genre() );
    }
    return QString::number( genres.count() );
}

AlbumCounter::AlbumCounter()
{
}

AlbumCounter::~AlbumCounter()
{
}

QString
AlbumCounter::value( const Meta::TrackList &tracks ) const
{
    QSet<Meta::AlbumPtr> albums;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        if( track->album() )
            albums.insert( track->album() );
    }
    return QString::number( albums.count() );
}

ComposerCounter::ComposerCounter()
{
}

ComposerCounter::~ComposerCounter()
{
}

QString
ComposerCounter::value( const Meta::TrackList &tracks ) const
{
    QSet<Meta::ComposerPtr> composers;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        if( track->composer() )
            composers.insert( track->composer() );
    }
    return QString::number( composers.count() );
}

YearCounter::YearCounter()
{
}

YearCounter::~YearCounter()
{
}

QString
YearCounter::value( const Meta::TrackList &tracks ) const
{
    QSet<Meta::YearPtr> years;
    foreach( const Meta::TrackPtr &track, tracks )
    {
        if( track->year() )
            years.insert( track->year() );
    }
    return QString::number( years.count() );
}


//CustomReturnValues

TitleReturnValue::TitleReturnValue()
{
}

TitleReturnValue::~TitleReturnValue()
{
}

QString
TitleReturnValue::value( const Meta::TrackPtr &track ) const
{
    return track->name();
}

UrlReturnValue::UrlReturnValue()
{
}

UrlReturnValue::~UrlReturnValue()
{
}

QString
UrlReturnValue::value( const Meta::TrackPtr &track ) const
{
    return track->playableUrl().url();
}

class ArtistReturnValue : public CustomReturnValue
{
    public:
        ArtistReturnValue() {}
        virtual ~ArtistReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->artist() ? track->artist()->name() : QString(); }
};

class AlbumReturnValue : public CustomReturnValue
{
    public:
        AlbumReturnValue() {}
        virtual ~AlbumReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->album() ? track->album()->name() : QString(); }
};

class ComposerReturnValue : public CustomReturnValue
{
    public:
        ComposerReturnValue() {}
        virtual ~ComposerReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->composer() ? track->composer()->name() : QString(); }
};

class GenreReturnValue : public CustomReturnValue
{
    public:
        GenreReturnValue() {}
        virtual ~GenreReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->genre() ? track->genre()->name() : QString(); }
};

class YearReturnValue : public CustomReturnValue
{
    public:
        YearReturnValue() {}
        virtual ~YearReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->year() ? track->year()->name() : QString(); }
};

class CommentReturnValue : public CustomReturnValue
{
    public:
        CommentReturnValue() {}
        virtual ~CommentReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return track->comment(); }
};

class TrackNrReturnValue : public CustomReturnValue
{
    public:
        TrackNrReturnValue() {}
        virtual ~TrackNrReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->trackNumber() ); }
};

class DiscNrReturnValue : public CustomReturnValue
{
    public:
        DiscNrReturnValue() {}
        virtual ~DiscNrReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->discNumber() ); }
};

class ScoreReturnValue : public CustomReturnValue
{
    public:
        ScoreReturnValue() {}
        virtual ~ScoreReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->score() ); }
};

class RatingReturnValue : public CustomReturnValue
{
    public:
        RatingReturnValue() {}
        virtual ~RatingReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->rating() ); }
};

class PlaycountReturnValue : public CustomReturnValue
{
    public:
        PlaycountReturnValue() {}
        virtual ~PlaycountReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->playCount() ); }
};

class LengthReturnValue : public CustomReturnValue
{
    public:
        LengthReturnValue() {}
        virtual ~LengthReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->length() / 1000 ); }
};

class BitrateReturnValue : public CustomReturnValue
{
    public:
        BitrateReturnValue() {}
        virtual ~BitrateReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->bitrate() ); }
};

class FileSizeReturnValue : public CustomReturnValue
{
    public:
        FileSizeReturnValue() {}
        virtual ~FileSizeReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->filesize() ); }
};

class SampleRateReturnValue : public CustomReturnValue
{
    public:
        SampleRateReturnValue() {}
        virtual ~SampleRateReturnValue() {}
        virtual QString value( const Meta::TrackPtr &track ) const { return QString::number( track->sampleRate() ); }
};

CustomReturnValue*
CustomValueFactory::returnValue( qint64 value )
{
    switch( value )
    {
        case Meta::valTitle:
        {
            return new TitleReturnValue();
        }
        case Meta::valUrl:
        {
            return new UrlReturnValue();
        }
        case Meta::valArtist:
        {
            return new ArtistReturnValue();
        }
        case Meta::valAlbum:
        {
            return new AlbumReturnValue();
        }
        case Meta::valGenre:
        {
            return new GenreReturnValue();
        }
        case Meta::valComposer:
        {
            return new ComposerReturnValue();
        }
        case Meta::valYear:
        {
            return new YearReturnValue();
        }
        case Meta::valComment:
        {
            return new CommentReturnValue();
        }
        case Meta::valTrackNr:
        {
            return new TrackNrReturnValue();
        }
        case Meta::valDiscNr:
        {
            return new DiscNrReturnValue();
        }
        case Meta::valScore:
        {
            return new ScoreReturnValue();
        }
        case Meta::valRating:
        {
            return new RatingReturnValue();
        }
        case Meta::valPlaycount:
        {
            return new PlaycountReturnValue();
        }
        case Meta::valLength:
        {
            return new LengthReturnValue();
        }
        case Meta::valBitrate:
        {
            return new BitrateReturnValue();
        }
        case Meta::valFilesize:
        {
            return new FileSizeReturnValue();
        }
        case Meta::valSamplerate:
        {
            return new SampleRateReturnValue();
        }
        default:
            return 0;
    }
}
