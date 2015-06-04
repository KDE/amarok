/****************************************************************************************
 * Copyright (c) 2011 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef LYRICS_BROWSER_H
#define LYRICS_BROWSER_H

#include <Plasma/TextBrowser>

namespace Plasma {
    class SvgWidget;
}

class LyricsBrowser : public Plasma::TextBrowser
{
    Q_OBJECT
    Q_PROPERTY( Qt::Alignment alignment READ alignment WRITE setAlignment )
    Q_PROPERTY( bool isReadOnly READ isReadOnly WRITE setReadOnly )
    Q_PROPERTY( bool isRichText READ isRichText WRITE setRichText )
    Q_PROPERTY( QString lyrics READ lyrics WRITE setLyrics )

public:
    explicit LyricsBrowser( QGraphicsWidget *parent = 0 );
    ~LyricsBrowser();

    Qt::Alignment alignment() const;
    bool isReadOnly() const;
    bool isRichText() const;
    QString lyrics() const;

    void clear();

    void setAlignment( Qt::Alignment alignment );
    void setLyrics( const QString &lyrics );
    void setReadOnly( bool readOnly );
    void setRichText( bool richText );

protected:
    void resizeEvent( QGraphicsSceneResizeEvent *event );

private Q_SLOTS:
    void paletteChanged( const QPalette &palette );
    void updateAlignment();

private:
    bool m_isRichText;
    Qt::Alignment m_alignment;
    Plasma::SvgWidget *m_topBorder;
    Plasma::SvgWidget *m_bottomBorder;
};

#endif // LYRICS_BROWSER_H
