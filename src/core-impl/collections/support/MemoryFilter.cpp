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

#include "MemoryFilter.h"

#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"

#include <QDateTime>

class UrlMemoryFilter : public StringMemoryFilter
{
protected:
    QString value( const Meta::TrackPtr &track ) const override
    { return track->playableUrl().url(); }

};

class GenericStringMemoryFilter : public StringMemoryFilter
{
    public:
        GenericStringMemoryFilter( qint64 value, const QString &filter,
                                   bool matchBegin, bool matchEnd )
        : m_value( value )
        { setFilter( filter, matchBegin, matchEnd ); }

    protected:
        QString value( const Meta::TrackPtr &track ) const override
        { return Meta::valueForField( m_value, track ).toString(); }

    private:
        qint64 m_value;
};

class GenericNumberMemoryFilter : public NumberMemoryFilter
{
    public:
        GenericNumberMemoryFilter( qint64 value, qint64 filter,
                                   Collections::QueryMaker::NumberComparison compare )
        : m_value( value )
        { setFilter( filter, compare ); }

    protected:
        qint64 value( const Meta::TrackPtr &track ) const override
        {
            QVariant v = Meta::valueForField( m_value, track );
            if( v.type() == QVariant::DateTime )
                return v.toDateTime().toSecsSinceEpoch();
            else
                return v.toLongLong();
        }

    private:
        qint64 m_value;
};

namespace FilterFactory
{

    MemoryFilter* filter( qint64 value, const QString &filter, bool matchBegin, bool matchEnd )
    {
        MemoryFilter *result = new GenericStringMemoryFilter( value, filter, matchBegin, matchEnd );
        return result;
    }

    MemoryFilter* numberFilter( qint64 value, qint64 filter, Collections::QueryMaker::NumberComparison compare )
    {
        NumberMemoryFilter *result = new GenericNumberMemoryFilter( value, filter, compare );
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
    if( filter )
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
        if( filter && !filter->filterMatches( track ) )
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

NumberMemoryFilter::NumberMemoryFilter()
    : MemoryFilter()
    , m_filter( 0 )
    , m_compare( Collections::QueryMaker::Equals )
{
}

NumberMemoryFilter::~NumberMemoryFilter()
{
}

void
NumberMemoryFilter::setFilter( qint64 filter, Collections::QueryMaker::NumberComparison compare )
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
        case Collections::QueryMaker::Equals:
            return currentValue == m_filter;
        case Collections::QueryMaker::GreaterThan:
            return currentValue > m_filter;
        case Collections::QueryMaker::LessThan:
            return currentValue < m_filter;
    }
    return false;
}

LabelFilter::LabelFilter( const QString &filter, bool matchBegin, bool matchEnd )
    : MemoryFilter()
{
    QString pattern;
    if( matchBegin )
        pattern += '^';
    pattern += filter;
    if( matchEnd )
        pattern += '$';

    m_expression = QRegularExpression( pattern, QRegularExpression::CaseInsensitiveOption );
}

LabelFilter::~LabelFilter()
{
    //nothing to do
}

bool
LabelFilter::filterMatches(const Meta::TrackPtr &track ) const
{
    for( const auto &label : track->labels() )
    {
        if( label->name().indexOf( m_expression ) != -1 )
            return true;
    }
    return false;
}
