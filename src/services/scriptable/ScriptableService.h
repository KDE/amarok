/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROKSCRIPTABLESERVICE_H
#define AMAROKSCRIPTABLESERVICE_H


#include "Amarok.h"
#include "../ServiceBase.h"
#include "ScriptableServiceMeta.h"
#include "ScriptableServiceCollection.h"


/* internally, we use the following level mapping:
    0 = track
    1 = album
    2 = artist
    3 = genre

but we do our best to make this invisible to the scripts
*/

typedef QMap<int, Meta::ScriptableServiceTrack *> ScriptableServiceTrackIdMap;
typedef QMap<int, Meta::ScriptableServiceArtist *> ScriptableServiceArtistIdMap;
typedef QMap<int, Meta::ScriptableServiceAlbum *> ScriptableServiceAlbumIdMap;
typedef QMap<int, Meta::ScriptableServiceGenre *> ScriptableServiceGenreIdMap;


class ScriptableService : public ServiceBase
{
    Q_OBJECT

public:

     /**
     * Constructor
     */
    ScriptableService( const QString &name );
    
    /**
     * Destructor
     */
    ~ScriptableService();

    void init( int levels, const QString &rootHtml, bool showSearchBar );

    void polish();

    ServiceCollection * collection();

    int insertItem( int level, int parentId, const QString &name, const QString &infoHtml, const QString &callbackData, const QString &playableUrl,
                    const QString & albumOverride, const QString & artistOverride, const QString & genreOverride,
                    const QString & composerOverride, int yearOverride, const QString &coverUrl );



    void donePopulating( int parentId );

    void setCustomEmblem( const QPixmap &emblem );
    QPixmap customEmblem();

    
    void setCustomScalableEmblem( const QString &emblemPath );
    QString customScalableEmblem();
    
    void setCurrentInfo( const QString & info );

    int contentLevels() { return m_levels; }
    bool hasSearchBar() { return m_hasSearchBar; }

private slots:


    //void treeItemSelected( const QModelIndex & index );
    //void infoChanged ( QString infoHtml );


private:

    bool m_polished;
    
    QString m_name;
    QString m_rootHtml;
            
    int m_levels;
    bool m_hasSearchBar;

    int addTrack( Meta::ScriptableServiceTrack * track );
    int addAlbum( Meta::ScriptableServiceAlbum * album );
    int addArtist( Meta::ScriptableServiceArtist * artist );
    int addGenre( Meta::ScriptableServiceGenre * genre );

    ScriptableServiceCollection * m_collection;
    int m_trackIdCounter;
    int m_albumIdCounter;
    int m_artistIdCounter;
    int m_genreIdCounter;

    ScriptableServiceTrackIdMap m_ssTrackIdMap;
    ScriptableServiceAlbumIdMap m_ssAlbumIdMap;
    ScriptableServiceArtistIdMap m_ssArtistIdMap;
    ScriptableServiceGenreIdMap m_ssGenreIdMap;

    QPixmap m_customEmblem;
    QString m_customScalableEmblem;
    
};


#endif
