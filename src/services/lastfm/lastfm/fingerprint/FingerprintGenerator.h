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

#ifndef FINGERPRINT_GENERATOR_H
#define FINGERPRINT_GENERATOR_H

#include "fplib/include/FingerprintExtractor.h"
#include "Sha256File.h"

#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

class FingerprintGenerator : public QThread
{
    Q_OBJECT
    
    public:
		
        enum Mode
        {
            /** Short query fingerprint is generated */
            Query,
            
            /** Full submittable query is generated */
            Full,
            
            Max
        };
        
        enum Error
        {
            /** File does not exist or cannot be read */
            ReadError = 0,
            
            /** GetInfo failed to extract samplerate, bitrate, channels, duration etc */
            GetInfoError,
            
            /** Track is shorter than minimum track duration for fingerprinting */
            TrackTooShortError,
            
            /** Could not initialize the fingerprintExtractor (probably ran out of RAM) */
            ExtractorInitError,
            
            /** The fingerprintExtractor has not been initialized before process() is called */
            ExtractorProcessError,
            
            /** The fingerprintExtractor has been starved of data to generate a fingerprint */
            ExtractorNotEnoughDataError,
            
            /** FingerprintExtractor::getFingerprint() has been called prematurely */
            ExtractorNotReadyError
        };
		
		FingerprintGenerator(const QFileInfo&, Mode, QObject* = 0 );
		
        void run();
                
        Mode mode() { return m_mode; }
		
        QByteArray& data() { return m_fingerprint; }
        QString sha256 ();
        
        void stop();
        
    signals:
        void failed( FingerprintGenerator::Error );
        void success( QByteArray);
		
    protected:
        //void fingerprintOld( QString path );
        void fingerprint( QString path );
        
        QFileInfo m_file;
		
        fingerprint::FingerprintExtractor m_extractor;
        QByteArray m_fingerprint;
        
    private:
        Mode m_mode;
        
        int m_sampleRate;
        int m_numChannels;
		
		
    private slots:
        void onStreamInitialized( long sampleRate, int channels );
        
		
};

#endif //FINGERPRINT_GENERATOR_H

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

#ifndef FINGERPRINT_GENERATOR_H
#define FINGERPRINT_GENERATOR_H

#include "fplib/include/FingerprintExtractor.h"
#include "Sha256File.h"

#include <QFileInfo>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>

class FingerprintGenerator : public QThread
{
    Q_OBJECT
    
    public:
		
        enum Mode
        {
            /** Short query fingerprint is generated */
            Query,
            
            /** Full submittable query is generated */
            Full,
            
            Max
        };
        
        enum Error
        {
            /** File does not exist or cannot be read */
            ReadError = 0,
            
            /** GetInfo failed to extract samplerate, bitrate, channels, duration etc */
            GetInfoError,
            
            /** Track is shorter than minimum track duration for fingerprinting */
            TrackTooShortError,
            
            /** Could not initialize the fingerprintExtractor (probably ran out of RAM) */
            ExtractorInitError,
            
            /** The fingerprintExtractor has not been initialized before process() is called */
            ExtractorProcessError,
            
            /** The fingerprintExtractor has been starved of data to generate a fingerprint */
            ExtractorNotEnoughDataError,
            
            /** FingerprintExtractor::getFingerprint() has been called prematurely */
            ExtractorNotReadyError
        };
		
		FingerprintGenerator(const QFileInfo&, Mode, QObject* = 0 );
		
        void run();
                
        Mode mode() { return m_mode; }
		
        QByteArray& data() { return m_fingerprint; }
        QString sha256 ();
        
        void stop();
        
    signals:
        void failed( FingerprintGenerator::Error );
        void success( QByteArray);
		
    protected:
        //void fingerprintOld( QString path );
        void fingerprint( QString path );
        
        QFileInfo m_file;
		
        fingerprint::FingerprintExtractor m_extractor;
        QByteArray m_fingerprint;
        
    private:
        Mode m_mode;
        
        int m_sampleRate;
        int m_numChannels;
		
		
    private slots:
        void onStreamInitialized( long sampleRate, int channels );
        
		
};

#endif //FINGERPRINT_GENERATOR_H

