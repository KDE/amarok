//
// C++ Interface: PrivateMetaRegistry
//
// Description: 
//
//
// Author:  <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef METAPRIVATEMETAREGISTRY_H
#define METAPRIVATEMETAREGISTRY_H

#include "Meta.h"

namespace Meta {

/**
An extremely simple registry used where tracks often have private often have private album (or other members) to corrolate these instead of creating a new one for each trac (even if they are from the same album). This, besides saving memory, also makes it possible to group by pointers in the playlist instead of doing some album/artist name foo.

	@author 
*/
class PrivateMetaRegistry{
public:

    static PrivateMetaRegistry * instance();

    void insertAlbum( const QString &owner, const QString &key, AlbumPtr album );
    void insertArtist( const QString &owner, const QString &key, ArtistPtr artist );
    void insertGenre( const QString &owner, const QString &key, GenrePtr genre );
    void insertComposer( const QString &owner, const QString &key, ComposerPtr composer );
    void insertYear( const QString &owner, const QString &key, YearPtr year );

    AlbumPtr album( const QString &owner, const QString &key );
    ArtistPtr artist( const QString &owner, const QString &key );
    GenrePtr genre( const QString &owner, const QString &key );
    ComposerPtr composer( const QString &owner, const QString &key );
    YearPtr year( const QString &owner, const QString &key );


private:

    PrivateMetaRegistry();
    ~PrivateMetaRegistry();

    static PrivateMetaRegistry* s_instance;      //! instance variable
    
    QMap<QString, AlbumPtr> m_albums;
    QMap<QString, ArtistPtr> m_artists;
    QMap<QString, GenrePtr> m_genre;
    QMap<QString, ComposerPtr> m_composers;
    QMap<QString, YearPtr> m_years;


};

}

#endif
