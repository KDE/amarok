/****************************************************************************************
 * Copyright (c) 2009 Jakob Kummerow <jakob.kummerow@gmail.com>                         *
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

#ifndef signer_H
#define signer_H

#include <QtCore/QObject>
#include <QtCrypto>

class signer : public QObject
{
<<<<<<< HEAD:utilities/updatesigner/signer.h
	Q_OBJECT
	public:
		signer();
		virtual ~signer();
		bool setParams(int argc, char** argv);
	
	public slots:
		void keygen();
		void signFile();
		void checkSignature();
		
	private:
		QString m_pubkeyFilename, m_privkeyFilename;
};

#endif // signer_H
=======

    class PlaylistFile;

    typedef KSharedPtr<PlaylistFile> PlaylistFilePtr;
    typedef QList<PlaylistFilePtr> PlaylistFileList;

    /**
     * Base class for all playlist files
     *
     **/
    class AMAROK_EXPORT PlaylistFile : public Playlist
    {
        public:
            PlaylistFile() {}
            PlaylistFile( const KUrl &url ) { Q_UNUSED( url ); }
            virtual ~PlaylistFile() {}

            virtual PlaylistProvider *provider() const { return m_provider; }

            virtual bool isWritable() { return false; }

            virtual bool save( const KUrl &url, bool relative )
                { Q_UNUSED( url ); Q_UNUSED( relative ); return false; }
            virtual bool load( QTextStream &stream ) { Q_UNUSED( stream ); return false; }

            virtual void setName( const QString &name ) = 0;
            virtual void setGroups( const QStringList &groups ) { Q_UNUSED( groups ); }
            virtual void setProvider( PlaylistProvider *provider ) { m_provider = provider; }
        private:
            PlaylistProvider *m_provider;
    };

}

Q_DECLARE_METATYPE( Meta::PlaylistFilePtr )
Q_DECLARE_METATYPE( Meta::PlaylistFileList )

#endif
>>>>>>> Add basic synchronization class and fuctions.:src/meta/PlaylistFile.h
