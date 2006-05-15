/***************************************************************************
                          playlist.h  -  description
                             -------------------
    begin                : 2001-07-24
    copyright            : (C) 2001 by Shaun Jackman (sjackman@debian.org)
    modify by:           : Andres Oton 
    email                : andres.oton@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __playlist_h__
#define __playlist_h__

// libnjb
#include <libnjb.h>

// Qt
#include <qstringlist.h>


class NjbPlaylist {
    public: 
        NjbPlaylist();
        NjbPlaylist( njb_playlist_t* playlist);
        NjbPlaylist(const NjbPlaylist& _copy);
        ~NjbPlaylist( void);

        QStringList trackNames( void) const;
        void operator=(const NjbPlaylist& _copy);
        bool operator==(const QString& name) const;
        bool operator==(const NjbPlaylist& rval) const;
        QString getName(void) const;

        int setName( const QString& fileName);
        int addTrack( const QString& fileName);
        int update( void);

        u_int32_t getId(void) const { return m_playlist->plid; }

    protected:
        void setPlaylist( njb_playlist_t* _newlist );

        static QString unescapefilename( const QString& );
        static QString escapefilename( const QString& );

    private:
        njb_playlist_t* m_playlist;
};

class playlistValueList: public QValueList<NjbPlaylist>
{
    public:
        int readFromDevice( void);
};

#endif
