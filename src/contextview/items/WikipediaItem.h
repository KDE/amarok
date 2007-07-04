/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WIKIPEDIA_ITEM_H
#define WIKIPEDIA_ITEM_H

#include "ContextItem.h"
#include "../ContextObserver.h"
#include "../GenericInfoBox.h"

#include <kio/job.h>

using namespace Context;

class WikipediaItem : public ContextItem, public ContextObserver
{
    Q_OBJECT

    static const int WIKI_MAX_HISTORY = 10;
    
public:
    
    WikipediaItem();
    
    void message( const QString& message );
    const QString name() { return "wikipedia"; }
    const QString shownDuring() { return "context"; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    
    void showWikipedia( const QString& url = QString(), bool fromHistory = false, bool replaceHistory = false );
    
private:
    
    QString wikiArtistPostfix();
    QString wikiAlbumPostfix();
    QString wikiTrackPostfix();
    QString wikiLocale();
    void setWikiLocale( const QString& );
    QString wikiURL( const QString& item );
    void reloadWikipedia();
    void showWikipediaEntry( const QString& entry, bool replaceHistory = false );
    
    
    GenericInfoBox *m_wikiBox;
    
    KJob* m_wikiJob;
    QString m_wikiCurrentEntry;
    QString m_wikiCurrentUrl;
    QString m_wikiBaseUrl;
    bool m_wikiVisible;
    QString m_wikiHTMLSource;
    QString m_wikiLanguages;
    
    QString m_wiki; // wiki source
    
    QStringList m_wikiBackHistory;
    QStringList m_wikiForwardHistory;
    
    static QString s_wikiLocale;
    
    bool    m_enabled;
private slots:
    
        /* these need resolution of the menu problem first
        void wikiConfigChanged( int );
        void wikiConfigApply();
        void wikiConfig();
        */
    void wikiArtistPage();
    void wikiAlbumPage();
    void wikiTitlePage();
    void wikiExternalPage();
    
    void wikiResult( KJob* job );
    
    
};

#endif
