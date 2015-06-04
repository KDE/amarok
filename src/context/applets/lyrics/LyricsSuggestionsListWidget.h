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

#ifndef LYRICS_SUGGESTIONS_LIST_WIDGET_H
#define LYRICS_SUGGESTIONS_LIST_WIDGET_H

#include <QUrl>
#include <Plasma/ScrollWidget>

class LyricsSuggestionItem;
class QGraphicsLinearLayout;

struct LyricsSuggestion
{
    QUrl url;
    QString title;
    QString artist;
};

class LyricsSuggestionsListWidget : public Plasma::ScrollWidget
{
    Q_OBJECT

public:
    explicit LyricsSuggestionsListWidget( QGraphicsWidget *parent = 0 );
    ~LyricsSuggestionsListWidget();

    void add( const LyricsSuggestion &suggestion );

    void clear();

Q_SIGNALS:
    void selected( const LyricsSuggestion &suggestion );

private:
    QList<LyricsSuggestionItem*> m_items;
    QList<QGraphicsWidget*> m_separators;
    QGraphicsLinearLayout *m_layout;
    Q_DISABLE_COPY( LyricsSuggestionsListWidget )
};

class LyricsSuggestionItem : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( QUrl url READ url )
    Q_PROPERTY( QString title READ title )
    Q_PROPERTY( QString artist READ artist )

public:
    LyricsSuggestionItem( const LyricsSuggestion &suggestion, QGraphicsItem *parent = 0 );
    ~LyricsSuggestionItem();

    QString artist() const;
    QString title() const;
    QUrl url() const;

Q_SIGNALS:
    void selected( const LyricsSuggestion &suggestion );

private Q_SLOTS:
    void onClicked();

private:
    LyricsSuggestion m_data;
    Q_DISABLE_COPY( LyricsSuggestionItem )
};

inline QString LyricsSuggestionItem::title() const
{ return m_data.title; }

inline QString LyricsSuggestionItem::artist() const
{ return m_data.artist; }

inline QUrl LyricsSuggestionItem::url() const
{ return m_data.url; }

Q_DECLARE_METATYPE( LyricsSuggestion )

#endif // LYRICS_SUGGESTIONS_LIST_WIDGET_H
