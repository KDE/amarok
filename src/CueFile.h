/**************************************************************************
*   Copyright (C) 2008 by Casey Link <unnamedrambler@gmail.com            *
*   Copyright (C) 2005 by Martin Ehmke <ehmke@gmx.de>                     *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#ifndef AMAROK_CUEFILE_H
#define AMAROK_CUEFILE_H

#include "amarok_export.h"
#include "EngineController.h"
#include "EngineObserver.h"

#include <QString>
#include <QMap>
#include <QObject>

class CueFileItem {
    public:
        CueFileItem (const QString& title, const QString& artist, const QString& album, const int trackNumber, const long index)
            : m_title( title )
            , m_artist( artist )
            , m_album( album )
            , m_trackNumber( trackNumber )
            , m_index( index )
            , m_length( -1 )
        {}

        CueFileItem()
            : m_title( )
            , m_artist( )
            , m_album( )
            , m_trackNumber( -1 )
            , m_index( -1 )
            , m_length( -1 )
        {}

        void setLength(const long length) { m_length = length; }

        const QString getTitle () const { return m_title; }
        const QString getArtist () const { return m_artist; }
        const QString getAlbum () const { return m_album; }
        int getTrackNumber () const { return m_trackNumber; }
        long getIndex () const { return m_index; }
        long getLength () const { return m_length; }

    private:
        QString m_title;
        QString m_artist;
        QString m_album;
        int     m_trackNumber;
        long    m_index;
        long    m_length;
};

// <<Singleton>>
class CueFile : public QObject, public QMap<long, CueFileItem>, public EngineObserver
{
        Q_OBJECT

    public:
        static CueFile *instance();

        void setCueFileName( QString name ) { m_cueFileName = name; };
        bool load(int mediaLength);

        // EngineObserver
        virtual void engineTrackPositionChanged( long /*position*/ , bool /*userSeek*/ );

    signals:
        /** Transmits new metadata bundle */
        //TODO 2.0 FIX void metaData( const MetaBundle& );
        /** Transmits new length information associated with current cue */
        void newCuePoint( long currentPos, long startPos, long endPos );

    protected:
        CueFile() : EngineObserver( The::engineController() ), m_lastSeekPos(-1) { };
        CueFile(EngineSubject *s) : EngineObserver(s), m_lastSeekPos(-1) { };
        ~CueFile();

    private:
        QString m_cueFileName;
        int m_lastSeekPos; // in seconds
};


#endif // AMAROK_CUEFILE_H
