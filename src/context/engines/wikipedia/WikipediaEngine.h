/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#ifndef AMAROK_WIKIPEDIA_ENGINE
#define AMAROK_WIKIPEDIA_ENGINE

#include "ContextObserver.h"
#include "core/meta/Meta.h"
#include "context/DataEngine.h"
#include "NetworkAccessManagerProxy.h"

#include <QLocale>

/**
    This class provide Wikipedia data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is the artist
           * the data is a QString containing the html of the wikipedia page
*/

using namespace Context;
class WikipediaEnginePrivate;

class WikipediaEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
public:
    WikipediaEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~WikipediaEngine();
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString& selection );
    QString selection() const;
    
protected:
    bool sourceRequestEvent( const QString& name );
    
private:
    WikipediaEnginePrivate *const d_ptr;
    Q_DECLARE_PRIVATE( WikipediaEngine )

    Q_PRIVATE_SLOT( d_ptr, void _wikiResult(const KUrl&,QByteArray,NetworkAccessManagerProxy::Error) )
};

K_EXPORT_AMAROK_DATAENGINE( wikipedia, WikipediaEngine )

#endif

