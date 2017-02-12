/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2014 Yash Ladia <yashladia1@gmail.com>                                 *
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

#ifndef LYRICS_APPLET_H
#define LYRICS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"

#include <ui_lyricsSettings.h>

class LyricsAppletPrivate;

class LyricsApplet : public Context::Applet
{
    Q_OBJECT

public:
    LyricsApplet( QObject* parent, const QVariantList& args );
    ~LyricsApplet();

    bool hasHeightForWidth() const;

public Q_SLOTS:
    virtual void init();
    void connectSource( const QString& source );
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
    void refreshLyrics();

protected:
    void createConfigurationInterface( KConfigDialog *parent );
    void keyPressEvent( QKeyEvent *e );

private:
    LyricsAppletPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( LyricsApplet )

    Q_PRIVATE_SLOT( d_ptr, void _editLyrics() )
    Q_PRIVATE_SLOT( d_ptr, void _changeLyricsFont() )
    Q_PRIVATE_SLOT( d_ptr, void _changeLyricsAlignment() )
    Q_PRIVATE_SLOT( d_ptr, void _closeLyrics() )
    Q_PRIVATE_SLOT( d_ptr, void _saveLyrics() )
    Q_PRIVATE_SLOT( d_ptr, void _toggleAutoScroll() )
    Q_PRIVATE_SLOT( d_ptr, void _suggestionChosen(LyricsSuggestion) )
    Q_PRIVATE_SLOT( d_ptr, void _unsetCursor() )
    Q_PRIVATE_SLOT( d_ptr, void _trackChanged( Meta::TrackPtr ) )
    Q_PRIVATE_SLOT( d_ptr, void _trackMetadataChanged( Meta::TrackPtr ) )
    Q_PRIVATE_SLOT( d_ptr, void _trackPositionChanged( qint64 position, bool userSeek ) )
    Q_PRIVATE_SLOT( d_ptr, void _lyricsChangedMessageButtonPressed(const Plasma::MessageButton) )
    Q_PRIVATE_SLOT( d_ptr, void _refetchMessageButtonPressed(const Plasma::MessageButton) )
};

AMAROK_EXPORT_APPLET( lyrics, LyricsApplet )

#endif
