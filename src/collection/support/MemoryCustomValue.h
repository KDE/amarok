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

#ifndef MEMORYCUSTOMVALUE_H
#define MEMORYCUSTOMVALUE_H

#include "amarok_export.h"
#include "core/meta/Meta.h"
#include "core/collections/QueryMaker.h"

#include <QList>
#include <QString>

class CustomReturnFunction;
class CustomReturnValue;

namespace CustomValueFactory
{
    CustomReturnFunction* returnFunction( Collections::QueryMaker::ReturnFunction function, qint64 value );
    CustomReturnValue* returnValue( qint64 value );
}

class AMAROK_EXPORT CustomReturnFunction
{
    public:
        CustomReturnFunction();
        virtual ~CustomReturnFunction();

        virtual QString value( const Meta::TrackList &tracks ) const = 0;
};

class AMAROK_EXPORT TrackCounter : public CustomReturnFunction
{
    public:
        TrackCounter();
        virtual ~TrackCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT ArtistCounter : public CustomReturnFunction
{
    public:
        ArtistCounter();
        virtual ~ArtistCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT GenreCounter : public CustomReturnFunction
{
    public:
        GenreCounter();
        virtual ~GenreCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT ComposerCounter : public CustomReturnFunction
{
    public:
        ComposerCounter();
        virtual ~ComposerCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT AlbumCounter : public CustomReturnFunction
{
    public:
        AlbumCounter();
        virtual ~AlbumCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT YearCounter : public CustomReturnFunction
{
    public:
        YearCounter();
        virtual ~YearCounter();

        virtual QString value( const Meta::TrackList &tracks ) const;
};

class AMAROK_EXPORT CustomReturnValue
{
    public:
        CustomReturnValue();
        virtual ~CustomReturnValue();

        virtual QString value( const Meta::TrackPtr &track ) const = 0;
};

class AMAROK_EXPORT TitleReturnValue : public CustomReturnValue
{
    public:
        TitleReturnValue();
        virtual ~TitleReturnValue();
        virtual QString value( const Meta::TrackPtr &track ) const;
};

class AMAROK_EXPORT UrlReturnValue : public CustomReturnValue
{
    public:
        UrlReturnValue();
        virtual ~UrlReturnValue();
        virtual QString value( const Meta::TrackPtr &track ) const;
};


#endif
