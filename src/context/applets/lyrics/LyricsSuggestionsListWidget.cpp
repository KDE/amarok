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

#define DEBUG_PREFIX "LyricsSuggestionsListWidget"

#include "LyricsSuggestionsListWidget.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QIcon>
#include <KSqueezedTextLabel>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <Plasma/Separator>

#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>

LyricsSuggestionsListWidget::LyricsSuggestionsListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
{
    QGraphicsWidget *viewport = new QGraphicsWidget( this );
    m_layout = new QGraphicsLinearLayout( Qt::Vertical, viewport );
    setWidget( viewport );
}

LyricsSuggestionsListWidget::~LyricsSuggestionsListWidget()
{}

void
LyricsSuggestionsListWidget::add( const LyricsSuggestion &suggestion )
{
    QGraphicsWidget *sep = new Plasma::Separator;
    LyricsSuggestionItem *item = new LyricsSuggestionItem( suggestion );
    item->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );
    m_layout->addItem( item );
    m_layout->addItem( sep );
    m_items.append( item );
    m_separators.append( sep );
    connect( item, SIGNAL(selected(LyricsSuggestion)), SIGNAL(selected(LyricsSuggestion)) );
}

void
LyricsSuggestionsListWidget::clear()
{
    qDeleteAll( m_items );
    qDeleteAll( m_separators );
    m_items.clear();
    m_separators.clear();
}

LyricsSuggestionItem::LyricsSuggestionItem( const LyricsSuggestion &suggestion, QGraphicsItem *parent )
    : QGraphicsWidget( parent )
    , m_data( suggestion )
{
    QGraphicsProxyWidget *titleProxy = new QGraphicsProxyWidget( this );
    KSqueezedTextLabel *titleLabel = new KSqueezedTextLabel( m_data.title );
    titleLabel->setTextElideMode( Qt::ElideRight );
    titleLabel->setAttribute( Qt::WA_NoSystemBackground );
    titleLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    titleProxy->setWidget( titleLabel );
    QFont font = titleLabel->font();
    font.setBold( true );
    titleLabel->setFont( font );

    const QUrl &url = m_data.url;
    QString urlText = QString("<a href=\"%1\">%2</a>").arg(url.url(), url.host());
    Plasma::Label *urlLabel = new Plasma::Label( this );
    urlLabel->setText( urlText );
    urlLabel->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
    urlLabel->nativeWidget()->setOpenExternalLinks( true );
    urlLabel->nativeWidget()->setTextInteractionFlags( Qt::TextBrowserInteraction );
    urlLabel->nativeWidget()->setToolTip( url.url() );

    QString artist = i18n( "artist: %1", m_data.artist );
    QGraphicsProxyWidget *artistProxy = new QGraphicsProxyWidget( this );
    KSqueezedTextLabel *artistLabel = new KSqueezedTextLabel( artist );
    artistLabel->setTextElideMode( Qt::ElideRight );
    artistLabel->setAttribute( Qt::WA_NoSystemBackground );
    artistLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    artistProxy->setWidget( artistLabel );

    Plasma::IconWidget *lyricsIcon( new Plasma::IconWidget(QIcon::fromTheme("amarok_lyrics"), QString(), this) );
    lyricsIcon->setDrawBackground( true );
    connect( lyricsIcon, SIGNAL(clicked()), SLOT(onClicked()) );

    QGraphicsGridLayout *layout = new QGraphicsGridLayout( this );
    layout->setVerticalSpacing( 0 );
    layout->addItem( lyricsIcon, 0, 0, 3, 1, Qt::AlignCenter );
    layout->addItem( titleProxy, 0, 1, Qt::AlignLeft );
    layout->addItem( artistProxy, 1, 1, Qt::AlignLeft );
    layout->addItem( urlLabel, 2, 1, Qt::AlignLeft );
}

LyricsSuggestionItem::~LyricsSuggestionItem()
{}

void
LyricsSuggestionItem::onClicked()
{
    emit selected( m_data );
}

