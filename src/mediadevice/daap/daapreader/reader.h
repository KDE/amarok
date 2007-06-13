/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu>              *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef DAAPREADER_H
#define DAAPREADER_H

#include <qobject.h>

#include <kurl.h>

class QString;

template <class T>
class QPtrList;
class MetaBundle;
class ServerItem;
class QHttpResponseHeader;

namespace Daap
{
    typedef QMap<QString, QVariant>    Map;

    typedef QPtrList< MetaBundle >     TrackList;
    typedef QMap< QString, TrackList > AlbumList;
    typedef QMap< QString, AlbumList > SongList;

//typedef QMap<QString, QMap<QString, QPtrList<MetaBundle> > > SongList;

    enum ContentTypes { INVALID = 0, CHAR = 1, SHORT = 2, LONG = 5, LONGLONG = 7,
                        STRING = 9, DATE = 10, DVERSION = 11, CONTAINER = 12 };
    struct Code
    {
        Code() : type(INVALID) { }
        Code( const QString& nName, ContentTypes nType ) : name( nName ), type( nType ) { }
        ~Code() { }
        QString name;
        ContentTypes type;
    };



    /**
        The nucleus of the DAAP client; composes queries and parses responses.
        @author Ian Monroe <ian@monroe.nu>
    */
    class Reader : public QObject
    {
        Q_OBJECT

        public:
            Reader( const QString& host, Q_UINT16 port, ServerItem* root,
                    const QString& password, QObject* parent, const char* name );
           ~Reader();

            //QPtrList<MetaBundle> getSongList();
            enum Options { SESSION_ID = 1, SERVER_VERSION = 2  };
            void loginRequest();
            void logoutRequest();
            ServerItem* rootMediaItem() const { return m_root; }

            int sessionId() const { return m_sessionId; }
            QString host() const { return m_host; }
            Q_UINT16 port() const { return m_port; }
        public slots:
            void logoutRequest(int, bool );
            void loginHeaderReceived( const QHttpResponseHeader& resp );
            void loginFinished( int id , bool error );
            void updateFinished( int id , bool error );
            void databaseIdFinished( int id , bool error );
            void songListFinished( int id, bool error );
            void fetchingError( const QString& error );

        signals:
            void daapBundles( const QString& host, Daap::SongList bundles );
            void httpError( const QString& );
            void passwordRequired();

        private:
            /**
            * Make a map-vector tree out of the DAAP binary result
            * @param raw stream of DAAP reply
            * @param containerLength length of the container (or entire result) being analyzed
            */
            static Map parse( QDataStream &raw, uint containerLength, bool first = false );
            static void addElement( Map &parentMap, char* tag, QVariant element ); //! supporter function for parse
            static Q_UINT32 getTagAndLength( QDataStream &raw, char tag[5] );

            static QMap<QString, Code> s_codes;

            QString m_host;
            Q_UINT16 m_port;
            QString m_loginString;
            QString m_databaseId;
            int m_sessionId;
            ServerItem* m_root;
            QString m_password;

    };

}

#endif
