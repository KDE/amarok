/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#ifndef WIKIPEDIA_APPLET_H
#define WIKIPEDIA_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "NetworkAccessManagerProxy.h"

class QAction;
class QDialog;
class KConfigDialog;
class QListWidgetItem;
class WikipediaAppletPrivate;

namespace Plasma
{
    class WebView;
    class IconWidget;
}

class WikipediaApplet : public Context::Applet
{
    Q_OBJECT

public:
    WikipediaApplet( QObject* parent, const QVariantList& args );
    ~WikipediaApplet();

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    
    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

public Q_SLOTS:
    virtual void init();
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
    void loadWikipediaUrl( const QString &url );

protected:
    void createConfigurationInterface(KConfigDialog *parent);

private:
    WikipediaAppletPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( WikipediaApplet )

    Q_PRIVATE_SLOT( d_ptr, void _goBackward() )
    Q_PRIVATE_SLOT( d_ptr, void _goForward() )
    Q_PRIVATE_SLOT( d_ptr, void _gotoAlbum() )
    Q_PRIVATE_SLOT( d_ptr, void _gotoArtist() )
    Q_PRIVATE_SLOT( d_ptr, void _gotoComposer() )
    Q_PRIVATE_SLOT( d_ptr, void _gotoTrack() )
    Q_PRIVATE_SLOT( d_ptr, void _linkClicked(const QUrl&) )
    Q_PRIVATE_SLOT( d_ptr, void _loadSettings() )
    Q_PRIVATE_SLOT( d_ptr, void _paletteChanged(const QPalette&) )
    Q_PRIVATE_SLOT( d_ptr, void _reloadWikipedia() )
    Q_PRIVATE_SLOT( d_ptr, void _updateWebFonts() )

    Q_PRIVATE_SLOT( d_ptr, void _getLangMapProgress(qint64,qint64) )
    Q_PRIVATE_SLOT( d_ptr, void _getLangMapFinished(const QUrl&,QByteArray,NetworkAccessManagerProxy::Error) )
    Q_PRIVATE_SLOT( d_ptr, void _getLangMap() )
    Q_PRIVATE_SLOT( d_ptr, void _configureLangSelector() )
    Q_PRIVATE_SLOT( d_ptr, void _langSelectorItemChanged(QListWidgetItem*) )

    Q_PRIVATE_SLOT( d_ptr, void _titleChanged(const QString&) )
    Q_PRIVATE_SLOT( d_ptr, void _pageLoadStarted() )
    Q_PRIVATE_SLOT( d_ptr, void _pageLoadProgress(int) )
    Q_PRIVATE_SLOT( d_ptr, void _pageLoadFinished(bool) )
    Q_PRIVATE_SLOT( d_ptr, void _searchLineEditTextEdited(const QString&) )
    Q_PRIVATE_SLOT( d_ptr, void _searchLineEditReturnPressed() )
    Q_PRIVATE_SLOT( d_ptr, void _jsWindowObjectCleared() )
};

AMAROK_EXPORT_APPLET( wikipedia, WikipediaApplet )

#endif
