/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef LYRICS_APPLET_H
#define LYRICS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

class TextScrollingWidget;
class QGraphicsSimpleTextItem;
class QGraphicsProxyWidget;
class QTextBrowser;

namespace Plasma
{
    class FrameSvg;
    class IconWidget;
}

class LyricsApplet : public Context::Applet
{
    Q_OBJECT

public:
    LyricsApplet( QObject* parent, const QVariantList& args );
    ~LyricsApplet();

    virtual void init();

    bool hasHeightForWidth() const;

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );    
    
public slots:
    void connectSource( const QString& source );
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
    void suggestionChosen( const QString& link );
    void refreshLyrics();
    
private slots:
    void paletteChanged( const QPalette & palette );
    void editLyrics();
    void saveLyrics();

private:
    void calculateHeight();
    void setEditing( const bool isEditing );

    void collapseToMin();

    QString m_titleText;
    TextScrollingWidget* m_titleLabel;

    Plasma::IconWidget*  m_saveIcon;
    Plasma::IconWidget*  m_editIcon;
    Plasma::IconWidget*  m_reloadIcon;
    
    // holds main body
    QGraphicsProxyWidget *m_lyricsProxy;
    QTextBrowser* m_lyrics;
    QGraphicsTextItem* m_suggested;

    QString m_lyricsTmpContent;
    bool m_hasLyrics;
};

K_EXPORT_AMAROK_APPLET( lyrics, LyricsApplet )

#endif
