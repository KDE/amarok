// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Stefan Bogner <bochi@online.ms>
// See COPYING file for licensing information.

#ifndef AMAROK_COVERFETCHER_H
#define AMAROK_COVERFETCHER_H

#include <qobject.h>     //baseclass
#include <qpixmap.h>     //baseclass
#include <qstring.h>     //stack alloc

namespace KIO {
    class Job;
}


class CoverFetcher : public QObject
{
    Q_OBJECT
    
    public:
        enum QueryMode { lite, heavy };
        
        CoverFetcher( const QString& license, QObject* parent = 0 );
        ~CoverFetcher();
        
        void setLicense( const QString& license ) { m_license = license; }
        void getCover( const QString& artist, const QString& album, const QString& saveas, QueryMode mode = lite, bool noedit = false, int size = 2, bool albumonly = false );
        
    signals:
        void imageReady( const QString& keyword, const QPixmap& image );        
        
    private slots:
        void xmlData( KIO::Job* job, const QByteArray& data );
        void xmlResult( KIO::Job* job );        
        void imageData( KIO::Job* job, const QByteArray& data );
        void imageResult( KIO::Job* job );        
        void editSearch();
        void saveCover();
        void saveCover( QPixmap pixmap );        
    private:
        static const uint BUFFER_SIZE = 2000000; // 2mb
        
        QString m_license;
        QString m_xmlDocument;
        QString m_keyword;
        QString m_artist;
        QString m_album;
        QString m_saveas;
        QString m_text;   
        QString m_url;
       
        uchar* m_buffer;
        int m_size;
        uint m_bufferIndex;
        QPixmap m_pixmap;
        bool m_noedit;
        bool m_albumonly;
};    
    

#endif /* AMAROK_COVERFETCHER_H */
