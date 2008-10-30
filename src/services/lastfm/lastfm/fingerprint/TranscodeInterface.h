/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

#ifndef TRANSCODE_INTERFACE_H
#define TRANSCODE_INTERFACE_H

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QLibrary>

class TranscodeInterface : public QObject
{
    public:
        virtual ~TranscodeInterface() {}

        virtual QStringList supportedMimeTypes() const = 0;
        virtual QStringList supportedFileExtensions() const = 0;

        virtual bool needsData() = 0;
        virtual bool hasData() = 0;

        virtual void setBufferCapacity( int bytes ) = 0;

        virtual int bufferSize() = 0;
        
        virtual void data( QByteArray& fillMe, int numBytes ) = 0;

        /**
         * Using the QPluginLoader::instance will always return the same
         * instance of the plugin. If you want a separate instance, use
         * this function and it will create and return a new instance.
         */
        virtual TranscodeInterface* newInstance() = 0;

        /**
         * When you've used newInstance, you should always call deleteInstance
         * for it when you're done with it.
         */
        virtual void deleteInstance( TranscodeInterface* i ) = 0;

    public slots:
        virtual void clearBuffers() = 0;

        /**
         * Slight hack. The noDecode causes it to just read ahead without decoding
         * any data. It's an optimisation used by the fingerprinter for skipping
         * ahead into files. Calling data afterwards will just return the correct
         * number of 0-filled bytes.
         */
        virtual bool processData( const QByteArray& data, bool noDecode = false ) = 0;

    signals:
        virtual void streamInitialized( long sampleRate, int channels ) = 0;

};

Q_DECLARE_INTERFACE( TranscodeInterface, "fm.last.Transcode/1.0" )

#endif
/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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

#ifndef TRANSCODE_INTERFACE_H
#define TRANSCODE_INTERFACE_H

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QPluginLoader>
#include <QLibrary>

class TranscodeInterface : public QObject
{
    public:
        virtual ~TranscodeInterface() {}

        virtual QStringList supportedMimeTypes() const = 0;
        virtual QStringList supportedFileExtensions() const = 0;

        virtual bool needsData() = 0;
        virtual bool hasData() = 0;

        virtual void setBufferCapacity( int bytes ) = 0;

        virtual int bufferSize() = 0;
        
        virtual void data( QByteArray& fillMe, int numBytes ) = 0;

        /**
         * Using the QPluginLoader::instance will always return the same
         * instance of the plugin. If you want a separate instance, use
         * this function and it will create and return a new instance.
         */
        virtual TranscodeInterface* newInstance() = 0;

        /**
         * When you've used newInstance, you should always call deleteInstance
         * for it when you're done with it.
         */
        virtual void deleteInstance( TranscodeInterface* i ) = 0;

    public slots:
        virtual void clearBuffers() = 0;

        /**
         * Slight hack. The noDecode causes it to just read ahead without decoding
         * any data. It's an optimisation used by the fingerprinter for skipping
         * ahead into files. Calling data afterwards will just return the correct
         * number of 0-filled bytes.
         */
        virtual bool processData( const QByteArray& data, bool noDecode = false ) = 0;

    signals:
        virtual void streamInitialized( long sampleRate, int channels ) = 0;

};

Q_DECLARE_INTERFACE( TranscodeInterface, "fm.last.Transcode/1.0" )

#endif
