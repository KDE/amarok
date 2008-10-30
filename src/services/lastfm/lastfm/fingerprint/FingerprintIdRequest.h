/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef FPID_FETCHER_H
#define FPID_FETCHER_H

#include "FingerprintGenerator.h"

#include "lib/types/Track.h"

#include "FingerprintDllExportMacro.h"

#include <QObject>
#include <QFileInfo>

/**
 *   @brief Check if the Fingerprint ID for the track has been cached in 
 *          the local collection db. If not it generates a Query fingerprint
 *          (using the FingerprintGenerator class) and emits one of the following signals:
 *          1) fpidFetched
 *          2) unknownFingerprint
 *
 **/
 
class FINGERPRINT_DLLEXPORT FingerprintIdRequest : public QObject
{
    Q_OBJECT
    
    public:

        FingerprintIdRequest( const Track&, QObject* parent = 0 );
        ~FingerprintIdRequest();
        
        const Track& track() const{ return m_track; }
        bool wait(){ if( m_fingerprinter ) return m_fingerprinter->wait(); }
        
    signals:
        /* This fingerprint has been found (either in the cache or from the servers ) */
        void FpIDFound( QString );
        
        /* This fingerprint was previously unknown - a new FPID gets assigned but
         * it's probably a good idea to submit the full fingerprint to the servers.
         */
        void unknownFingerprint( QString );
        
        /**
          * The track was found in the local collection database a queuedconnection
          * to FpIDFound will also be made. Unless you want to handle cached FpID's differently, 
          * use the FpIDFound signal.
          **/
        void cachedFpIDFound( QString );
        
        void fingerprintError();
        void networkError( QNetworkReply::NetworkError, QString );
        
    protected slots:
        void onFingerprintSuccess( const QByteArray& );
        void onFingerprintQueryFetched();
        
    private:
        FingerprintGenerator* m_fingerprinter;

        void fingerprint();

        Track m_track;
		QNetworkAccessManager* m_networkManager;
};

#endif //FPID_FETCHER_H
/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef FPID_FETCHER_H
#define FPID_FETCHER_H

#include "FingerprintGenerator.h"

#include "lib/types/Track.h"

#include "FingerprintDllExportMacro.h"

#include <QObject>
#include <QFileInfo>

/**
 *   @brief Check if the Fingerprint ID for the track has been cached in 
 *          the local collection db. If not it generates a Query fingerprint
 *          (using the FingerprintGenerator class) and emits one of the following signals:
 *          1) fpidFetched
 *          2) unknownFingerprint
 *
 **/
 
class FINGERPRINT_DLLEXPORT FingerprintIdRequest : public QObject
{
    Q_OBJECT
    
    public:

        FingerprintIdRequest( const Track&, QObject* parent = 0 );
        ~FingerprintIdRequest();
        
        const Track& track() const{ return m_track; }
        bool wait(){ if( m_fingerprinter ) return m_fingerprinter->wait(); }
        
    signals:
        /* This fingerprint has been found (either in the cache or from the servers ) */
        void FpIDFound( QString );
        
        /* This fingerprint was previously unknown - a new FPID gets assigned but
         * it's probably a good idea to submit the full fingerprint to the servers.
         */
        void unknownFingerprint( QString );
        
        /**
          * The track was found in the local collection database a queuedconnection
          * to FpIDFound will also be made. Unless you want to handle cached FpID's differently, 
          * use the FpIDFound signal.
          **/
        void cachedFpIDFound( QString );
        
        void fingerprintError();
        void networkError( QNetworkReply::NetworkError, QString );
        
    protected slots:
        void onFingerprintSuccess( const QByteArray& );
        void onFingerprintQueryFetched();
        
    private:
        FingerprintGenerator* m_fingerprinter;

        void fingerprint();

        Track m_track;
		QNetworkAccessManager* m_networkManager;
};

#endif //FPID_FETCHER_H
