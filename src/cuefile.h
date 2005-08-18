// (c) 2005 Martin Ehmke <ehmke@gmx.de>
// License: GNU General Public License V2

#ifndef CUEFILE_H
#define CUEFILE_H

#include <qstring.h>
#include <qmap.h>

#include <qobject.h>
#include "engineobserver.h"

class CueFileItem {
    public:
        CueFileItem (const QString& title, const QString& artist, const int trackNumber, const long index)
            : m_title( title )
            , m_artist( artist )
            , m_trackNumber( trackNumber )
            , m_index( index )

        {}

        CueFileItem () {};

        const QString getTitle () const { return m_title; }
        const QString getArtist () const { return m_artist; }
        const int getTrackNumber () const { return m_trackNumber; }
        const long getIndex () const { return m_index; }

    private:
        QString m_title;
        QString m_artist;
        int     m_trackNumber;
        long    m_index;
};

class CueFile : public QObject, public QMap<long, CueFileItem>, public EngineObserver
{
        Q_OBJECT

    public:

        CueFile() : EngineObserver(), m_lastSeekPos(-1) { };
        CueFile(EngineSubject *s) : EngineObserver(s), m_lastSeekPos(-1) { };

        ~CueFile(){ };
        void setCueFileName( QString name ) { m_cueFileName = name; };
        bool load();

        // EngineObserver
        virtual void engineTrackPositionChanged( long /*position*/ , bool /*userSeek*/ );

    signals:
        /** Transmits new metadata bundle */
        void metaData( const MetaBundle& );

    private:
        QString m_cueFileName;
        int m_lastSeekPos; // in seconds
};


#endif
