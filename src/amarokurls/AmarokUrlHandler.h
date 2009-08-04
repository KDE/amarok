/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AMAROKURLHANDLER_H
#define AMAROKURLHANDLER_H

#include "amarok_export.h"
#include "AmarokUrlRunnerBase.h"
#include "Meta.h"

#include <KIcon>

#include <QMap>


class TimecodeObserver;
class AmarokUrlHandler;
class NavigationUrlRunner;
class PlayUrlRunner;

namespace The {
    AMAROK_EXPORT AmarokUrlHandler* amarokUrlHandler();
}

/**
A singleton class for handling and delegating all amarok:// urls

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class AMAROK_EXPORT AmarokUrlHandler : public QObject
{
    Q_OBJECT
    friend AmarokUrlHandler* The::amarokUrlHandler();

public:

    void registerRunner( AmarokUrlRunnerBase * runner, const QString & command );
    void unRegisterRunner( AmarokUrlRunnerBase * runner );

    bool run( AmarokUrl url );

    BookmarkList urlsByCommand( const QString &command );
    KIcon iconForCommand( const QString &command );


public slots:
    void bookmarkAlbum( Meta::AlbumPtr album );
    void bookmarkArtist( Meta::ArtistPtr artist );

    void bookmarkCurrentBrowserView();
    void modelChanged();

signals:

    void urlsChanged();

private:

    AmarokUrlHandler();
    ~AmarokUrlHandler();

    QMap<QString, AmarokUrlRunnerBase *> m_registeredRunners;

    NavigationUrlRunner * m_navigationRunner;
    PlayUrlRunner * m_playRunner;
    TimecodeObserver * m_timecodeObserver;

};

#endif
