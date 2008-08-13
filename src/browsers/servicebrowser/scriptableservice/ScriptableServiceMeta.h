/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#ifndef DYNAMICSCRIPTABLESERVICEMETA_H
#define DYNAMICSCRIPTABLESERVICEMETA_H

#include "../ServiceMetaBase.h"

/**
meta types for use in the dynamic scriptable service. Nearly identical to the ServiceMetaBse types, except for a field for storing data to pass to the script to fetch child items for Genres, Artists and Albums and a level specifier

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/


namespace Meta
{

class ScriptableServiceMetaItem
{

    public:
        ScriptableServiceMetaItem( int level );
    
        void setCallbackString( const QString &callbackString );
        QString callbackString() const;
        int level() const;
    
    private:
    
        /* this is arbitrary string data to pass back to the script. This can be whatever
        information the script needs to fetch the children of this item... */
        QString m_callbackString;
        int m_level;
};


class ScriptableServiceTrack : public Meta::ServiceTrack, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceTrack( const QString & name, const QString & url );
        ScriptableServiceTrack( const QStringList & resultRow );

};

class ScriptableServiceAlbum : public Meta::ServiceAlbum, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceAlbum( const QString & name );
        ScriptableServiceAlbum( const QStringList & resultRow );

};

class ScriptableServiceArtist : public Meta::ServiceArtist, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceArtist( const QString & name );
        ScriptableServiceArtist( const QStringList & resultRow );

        void setGenreId( int artistId );
        int genreId( ) const;

    private:

        int m_genreId;

};


class ScriptableServiceGenre : public Meta::ServiceGenre, public ScriptableServiceMetaItem
{
    public:
        ScriptableServiceGenre( const QString & name );
        ScriptableServiceGenre( const QStringList & resultRow );

        void setDescription( const QString &description );
        QString description();

    private:
        QString m_description;

};

}



#endif
