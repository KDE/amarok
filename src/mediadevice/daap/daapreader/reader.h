/***************************************************************************
 * copyright            : (C) 2006 Ian Monroe <ian@monroe.nu> 
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy 
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/
#ifndef DAAPREADER_H
#define DAAPREADER_H

#include <QObject>
//Added by qt3to4:
#include <Q3PtrList>

#include <kurl.h>

class QString;

template <class T>
class Q3PtrList;
class MetaBundle;
class ServerItem;
class Q3HttpResponseHeader;

namespace Q3Http {
    class Error;
}

namespace Daap
{
    typedef QMap<QString, QVariant>    Map;

    typedef Q3PtrList< MetaBundle >     TrackList;
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
            Reader( const QString& host, quint16 port, ServerItem* root,
                    const QString& password, QObject* parent, const char* name );
           ~Reader();

            //QPtrList<MetaBundle> getSongList();
            enum Options { SESSION_ID = 1, SERVER_VERSION = 2  };
            void loginRequest();
            void logoutRequest();
            ServerItem* rootMediaItem() const { return m_root; }

            int sessionId() const { return m_sessionId; }
            QString host() const { return m_host; }
            quint16 port() const { return m_port; }
        public slots:
            void logoutRequest(int, bool );
            void loginHeaderReceived( const Q3HttpResponseHeader& resp );
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
            static quint32 getTagAndLength( QDataStream &raw, char tag[5] );

            static QMap<QString, Code> s_codes;

            QString m_host;
            quint16 m_port;
            QString m_loginString;
            QString m_databaseId;
            int m_sessionId;
            ServerItem* m_root;
            QString m_password;

    };

}

#endif
