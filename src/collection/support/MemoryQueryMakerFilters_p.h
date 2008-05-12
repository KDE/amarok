/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_MEMORYQUERYMAKERFILTERS_P_H
#define AMAROK_MEMORYQUERYMAKERFILTERS_P_H

#include "Debug.h"
#include "meta/Meta.h"
#include "collection/QueryMaker.h"

#include <QList>
#include <QString>

class Filter
{
public:
    Filter() {}
    virtual ~Filter() {}
    virtual bool filterMatches( Meta::TrackPtr track ) const = 0;
};

class ContainerFilter : public Filter
{
public:
    ContainerFilter() : Filter() {}
    virtual ~ContainerFilter() {}
    virtual void addFilter( Filter *filter ) = 0;
};

class AndFilter : public ContainerFilter
{
public:
    AndFilter() : ContainerFilter() {}
    virtual ~AndFilter()
    {
        qDeleteAll( m_filters );
        m_filters.clear();
    }

    virtual void addFilter( Filter *filter )
    {
        m_filters.append( filter );
    }

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        foreach( Filter *filter, m_filters )
        {
            if( !filter->filterMatches( track ) )
            {
                return false;
            }
        }
        return true;
    }

private:
    QList<Filter*> m_filters;
};

class OrFilter : public ContainerFilter
{
public:
    OrFilter() : ContainerFilter() {}
    virtual ~OrFilter()
    {
        qDeleteAll( m_filters );
        m_filters.clear();
    }

    virtual void addFilter( Filter *filter )
    {
        m_filters.append( filter );
    }

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        foreach( Filter *filter, m_filters )
        {
            if( filter->filterMatches( track ) )
            {
                return true;
            }
        }
        return false;
    }

private:
    QList<Filter*> m_filters;
};

class NegateFilter : public Filter
{
public:
    NegateFilter( Filter *filter ) : Filter(), m_filter( filter ) {}
    virtual ~NegateFilter() { delete m_filter; }
    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        return !m_filter->filterMatches( track );
    }

private:
    Filter *m_filter;
};

class TrackNameFilter : public Filter
{
public:
    TrackNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~TrackNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        DEBUG_BLOCK
        if( m_matchBegin && m_matchEnd )
            return track->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

class ArtistNameFilter : public Filter
{
public:
    ArtistNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~ArtistNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        if( m_matchBegin && m_matchEnd )
            return track->artist()->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->artist()->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->artist()->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->artist()->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

class AlbumNameFilter : public Filter
{
public:
    AlbumNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~AlbumNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        if( m_matchBegin && m_matchEnd )
            return track->album()->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->album()->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->album()->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->album()->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

class GenreNameFilter : public Filter
{
public:
    GenreNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~GenreNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        if( m_matchBegin && m_matchEnd )
            return track->genre()->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->genre()->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->genre()->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->genre()->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

class ComposerNameFilter : public Filter
{
public:
    ComposerNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~ComposerNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        if( m_matchBegin && m_matchEnd )
            return track->composer()->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->composer()->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->composer()->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->composer()->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

class YearNameFilter : public Filter
{
public:
    YearNameFilter( const QString &name, bool matchBegin, bool matchEnd ) : Filter()
    {
        m_name = name;
        m_matchBegin = matchBegin;
        m_matchEnd = matchEnd;
    }
    virtual ~YearNameFilter() {}

    virtual bool filterMatches( Meta::TrackPtr track ) const
    {
        if( m_matchBegin && m_matchEnd )
            return track->year()->name().compare( m_name, Qt::CaseInsensitive ) == 0;
        else if( m_matchBegin )
            return track->year()->name().startsWith( m_name, Qt::CaseInsensitive );
        else if( m_matchEnd )
            return track->year()->name().endsWith( m_name, Qt::CaseInsensitive );
        else
            return track->year()->name().contains( m_name, Qt::CaseInsensitive );
    }

private:
    QString m_name;
    bool m_matchBegin;
    bool m_matchEnd;
};

namespace FilterFactory
{
    Filter* filter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        Filter *result = 0;
        switch( value )
        {
            case QueryMaker::valTitle:
            {
                result = new TrackNameFilter( filter, matchBegin, matchEnd );
                break;
            }
            case QueryMaker::valAlbum:
            {
                result = new AlbumNameFilter( filter, matchBegin, matchEnd );
                break;
            }
            case QueryMaker::valArtist:
            {
                result = new ArtistNameFilter( filter, matchBegin, matchEnd );
                break;
            }
            case QueryMaker::valYear:
            {
                result = new YearNameFilter( filter, matchBegin, matchEnd );
                break;
            }
            case QueryMaker::valComposer:
            {
                result = new ComposerNameFilter( filter, matchBegin, matchEnd );
                break;
            }
        }
        return result;
    }
}

#endif
