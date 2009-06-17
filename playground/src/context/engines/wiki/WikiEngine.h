/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 * copyright            : (C) 2008 Mark Kretschmann <kretschmann@kde.org>  *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_WIKI_ENGINE
#define AMAROK_WIKI_ENGINE

#include "ContextObserver.h"
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <KIO/Job>
#include <QLocale>

/**
    This class provide Wikipedia data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is the artist
           * the data is a QString containing the html of the wikipedia page
*/

using namespace Context;

class WikiEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
public:
    WikiEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~WikiEngine();
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString& selection ) { m_currentSelection = selection; }
    QString selection() { return m_currentSelection; }
    
protected:
    bool sourceRequestEvent( const QString& name );
    
private slots:
    void albumResult(KJob *job);
    void titleResult(KJob *job);
    void artistResult(KJob *job);
    
private:
    void update();
    void updateArtistInfo();
    void updateAlbumInfo();
    void updateTitleInfo();

    void parse(QString& wiki);
    
    QString wikiArtistPostfix();
    QString wikiAlbumPostfix();
    QString wikiTrackPostfix();
    QString wikiSiteUrl();
    QString wikiUrl( const QString& item ) const;
    QString wikiLocale() const;
    
    void reloadArtistInfo();
    void reloadAlbumInfo();
    void reloadTitleInfo();
    void reloadAll();
    KJob *m_artistJob,*m_titleJob,*m_albumJob;

    Meta::TrackPtr m_currentTrack;
    QString m_currentSelection;
    QString m_wikiCurrentArtistEntry,m_wikiCurrentAlbumEntry,m_wikiCurrentTitleEntry;
    QString m_artistUrl,m_titleUrl,m_albumUrl;
    QString m_wikiLanguages;
    QLocale m_wikiLang;
    // stores what features are enabled
    bool m_requested;
    QStringList m_sources;
    bool m_triedRefinedSearchArtist,m_triedRefinedSearchAlbum,m_triedRefinedSearchTitle;
};

K_EXPORT_AMAROK_DATAENGINE( wiki, WikiEngine )

#endif

