/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROKURLHANDLER_H
#define AMAROKURLHANDLER_H

#include "amarok_export.h"
#include "AmarokUrlGenerator.h"
#include "AmarokUrlRunnerBase.h"
#include "Meta.h"
#include "playlist/PlaylistViewUrlRunner.h"

#include <KIcon>

#include <QMap>


class TimecodeObserver;
class AmarokUrlHandler;
class NavigationUrlRunner;
class PlayUrlRunner;

namespace The
{
AMAROK_EXPORT AmarokUrlHandler* amarokUrlHandler();
}

/**
A singleton class for handling and delegating all amarok:// urls

 @author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class AMAROK_EXPORT AmarokUrlHandler : public QObject
{
    Q_OBJECT
    friend AmarokUrlHandler* The::amarokUrlHandler();

public:

    void registerRunner( AmarokUrlRunnerBase * runner, const QString & command );
    void unRegisterRunner( AmarokUrlRunnerBase * runner );

    void registerGenerator( AmarokUrlGenerator * generator );
    void unRegisterGenerator( AmarokUrlGenerator * generator );

    bool run( AmarokUrl url );

    BookmarkList urlsByCommand( const QString &command );
    KIcon iconForCommand( const QString &command );

    void updateTimecodes( const QString * BookmarkName = 0 );
    void paintNewTimecode( const QString &name, int pos );

    QList<AmarokUrlGenerator *> generators() { return m_registeredGenerators; }


public slots:
    void bookmarkAlbum( Meta::AlbumPtr album );
    void bookmarkArtist( Meta::ArtistPtr artist );

    void bookmarkCurrentBrowserView();
    void bookmarkCurrentPlaylistView();
    void bookmarkCurrentContextView();

signals:
    void timecodesUpdated( const QString * BookmarkName );
    void timecodeAdded( const QString &name, int pos );

private:

    AmarokUrlHandler();
    ~AmarokUrlHandler();

    QMap<QString, AmarokUrlRunnerBase *> m_registeredRunners;
    QList<AmarokUrlGenerator *> m_registeredGenerators;

    NavigationUrlRunner * m_navigationRunner;
    Playlist::ViewUrlRunner * m_playlistViewRunner;
    PlayUrlRunner * m_playRunner;
    TimecodeObserver * m_timecodeObserver;

};

#endif
