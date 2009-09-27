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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEMORYFILTER_H
#define MEMORYFILTER_H

#include "amarok_export.h"
#include "Meta.h"
#include "QueryMaker.h"

#include <QList>
#include <QString>

class MemoryFilter;

namespace FilterFactory
{
    MemoryFilter* filter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd );
    MemoryFilter* numberFilter( qint64 value, qint64 filter, QueryMaker::NumberComparison compare );
}

class AMAROK_EXPORT MemoryFilter
{
    public:
        MemoryFilter();
        virtual ~MemoryFilter();
        virtual bool filterMatches( const Meta::TrackPtr &track ) const = 0;
};

class AMAROK_EXPORT ContainerMemoryFilter : public MemoryFilter
{
    public:
        ContainerMemoryFilter();
        virtual ~ContainerMemoryFilter();
        void addFilter( MemoryFilter *filter );
    protected:
        QList<MemoryFilter*> m_filters;
};

class AMAROK_EXPORT AndContainerMemoryFilter : public ContainerMemoryFilter
{
    public:
        AndContainerMemoryFilter();
        virtual ~AndContainerMemoryFilter();
        virtual bool filterMatches( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT OrContainerMemoryFilter : public ContainerMemoryFilter
{
    public:
        OrContainerMemoryFilter();
        virtual ~OrContainerMemoryFilter();
        virtual bool filterMatches( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT NegateMemoryFilter : public MemoryFilter
{
    public:
        NegateMemoryFilter( MemoryFilter *filter );
        virtual ~NegateMemoryFilter();
        virtual bool filterMatches( const Meta::TrackPtr &track ) const;
    private:
        MemoryFilter *m_filter;
};

class AMAROK_EXPORT StringMemoryFilter : public MemoryFilter
{
    public:
        StringMemoryFilter();
        virtual ~StringMemoryFilter();
        virtual bool filterMatches( const Meta::TrackPtr &track ) const;

        void setFilter( const QString &filter, bool matchBegin, bool matchEnd );
    protected:
        virtual QString value( const Meta::TrackPtr &track ) const = 0;

    private:
        QString m_filter;
        bool m_matchBegin;
        bool m_matchEnd;
};

class AMAROK_EXPORT TitleMemoryFilter : public StringMemoryFilter
{
    public:
        TitleMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~TitleMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT ArtistMemoryFilter : public StringMemoryFilter
{
    public:
        ArtistMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~ArtistMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT AlbumMemoryFilter : public StringMemoryFilter
{
    public:
        AlbumMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~AlbumMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT AlbumArtistMemoryFilter : public StringMemoryFilter
{
    public:
        AlbumArtistMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~AlbumArtistMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT GenreMemoryFilter : public StringMemoryFilter
{
    public:
        GenreMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~GenreMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT ComposerMemoryFilter : public StringMemoryFilter
{
    public:
        ComposerMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~ComposerMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT YearMemoryFilter : public StringMemoryFilter
{
    public:
        YearMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~YearMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT CommentMemoryFilter : public StringMemoryFilter
{
    public:
        CommentMemoryFilter( const QString &filter, bool matchBegin, bool matchEnd );
        virtual ~CommentMemoryFilter();

    protected:
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT NumberMemoryFilter : public MemoryFilter
{
    public:
        NumberMemoryFilter();
        virtual ~NumberMemoryFilter();
        void setFilter( qint64 filter, QueryMaker::NumberComparison compare );
        bool filterMatches( const Meta::TrackPtr &track ) const;
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const = 0;
    private:
        qint64 m_filter;
        QueryMaker::NumberComparison m_compare;
};

class AMAROK_EXPORT TrackNumberFilter : public NumberMemoryFilter
{
    public:
        TrackNumberFilter();
        virtual ~TrackNumberFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT DiscNumberFilter : public NumberMemoryFilter
{
    public:
        DiscNumberFilter();
        virtual ~DiscNumberFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT RatingFilter : public NumberMemoryFilter
{
    public:
        RatingFilter();
        virtual ~RatingFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT ScoreFilter : public NumberMemoryFilter
{
    public:
        ScoreFilter();
        virtual ~ScoreFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT PlaycountFilter : public NumberMemoryFilter
{
    public:
        PlaycountFilter();
        virtual ~PlaycountFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT FirstPlayedFilter : public NumberMemoryFilter
{
    public:
        FirstPlayedFilter();
        virtual ~FirstPlayedFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT LastPlayedFilter : public NumberMemoryFilter
{
    public:
        LastPlayedFilter();
        virtual ~LastPlayedFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT LengthFilter : public NumberMemoryFilter
{
    public:
        LengthFilter();
        virtual ~LengthFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT FilesizeFilter : public NumberMemoryFilter
{
    public:
        FilesizeFilter();
        virtual ~FilesizeFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT SampleRateFilter : public NumberMemoryFilter
{
    public:
        SampleRateFilter();
        virtual ~SampleRateFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT BitrateFilter : public NumberMemoryFilter
{
    public:
        BitrateFilter();
        virtual ~BitrateFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT CreateDateFilter : public NumberMemoryFilter
{
    public:
        CreateDateFilter();
        virtual ~CreateDateFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT YearNumberFilter : public NumberMemoryFilter
{
    public:
        YearNumberFilter();
        virtual ~YearNumberFilter();
    protected:
        virtual qint64 value( const Meta::TrackPtr &track ) const;
};

#endif
