/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *   Copyright (C) 2009-2010 Oleksandr Khayrullin <saniokh@gmail.com>          *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "ToolTipManager.h"

#include "amarokconfig.h"

#include "AmarokToolTip.h"
#include "core/meta/Meta.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/layouts/LayoutManager.h"
#include "playlist/layouts/LayoutItemConfig.h"

#include "KToolTip.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>
#include <KIcon>
#include <KIconLoader>

ToolTipManager::ToolTipManager(QAbstractItemView* parent) :
    QObject(parent),
    m_view(parent),
    m_timer(0),
    m_itemRect()
{
    g_delegate = new AmarokBalloonTooltipDelegate();
    KToolTip::setToolTipDelegate(g_delegate);

    connect(parent, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(requestToolTip(const QModelIndex&)));
    connect(parent, SIGNAL(viewportEntered()),
            this, SLOT(hideToolTip()));

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(prepareToolTip()));

    // When the mousewheel is used, the items don't get a hovered indication
    // (Qt-issue #200665). To assure that the tooltip still gets hidden,
    // the scrollbars are observed.
    connect(parent->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));

    m_view->viewport()->installEventFilter(this);

    cancelExclusions();
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::hideTip()
{
    hideToolTip();
}

bool ToolTipManager::eventFilter(QObject* watched, QEvent* event)
{
    if ((watched == m_view->viewport()) && (event->type() == QEvent::Leave)) {
        hideToolTip();
    }

    return QObject::eventFilter(watched, event);
}

void ToolTipManager::requestToolTip(const QModelIndex& index)
{
    // only request a tooltip for the name column and when no selection or
    // drag & drop operation is done (indicated by the left mouse button)
    Meta::TrackPtr track = index.data( Playlist::TrackRole ).value<Meta::TrackPtr>();
    if ( ( Playlist::LayoutManager::instance()->activeLayout().tooltips() ) && (track) && !(QApplication::mouseButtons() & Qt::LeftButton)) {
        m_track = track;
        KToolTip::hideTip();

        m_singleItem = (index.data( Playlist::GroupRole ).toInt() == Playlist::None);

        m_itemRect = m_view->visualRect(index);
        const QPoint pos = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);

        m_timer->start(750);
    } else {
        hideToolTip();
    }
}

void ToolTipManager::hideToolTip()
{
    m_timer->stop();
    KToolTip::hideTip();
}

void ToolTipManager::prepareToolTip()
{
/*    if (m_generatingPreview) {
        m_waitOnPreviewTimer->start(250);
    }*/

    QPixmap image;
    if ( ( m_track->album()->hasImage() ) && ( isVisible(Playlist::CoverImage) ) )
    {
        image = m_track->album()->image();
    }
    else
    {
        image = QPixmap();
    }

    QString text;


    if ((isVisible(Playlist::Artist)) && (m_track->artist()))
    {
        text += HTMLLine( Playlist::Artist, m_track->artist()->prettyName() );
    }
    if ((isVisible(Playlist::Album)) && (m_track->album()))
    {
        text += HTMLLine( Playlist::Album, m_track->album()->prettyName() );
    }
    if (isVisible(Playlist::DiscNumber))
    {
        text += HTMLLine( Playlist::DiscNumber, m_track->discNumber() );
    }
    if (isVisible(Playlist::Title))
    {
        text += HTMLLine( Playlist::Title, m_track->prettyName() );
    }
    if (isVisible(Playlist::TrackNumber))
    {
        text += HTMLLine( Playlist::TrackNumber, m_track->trackNumber() );
    }
    if ((isVisible(Playlist::Composer)) && (m_track->composer()))
    {
        text += HTMLLine( Playlist::Composer, m_track->composer()->prettyName() );
    }
    if ((isVisible(Playlist::Genre)) && (m_track->genre()))
    {
        text += HTMLLine( Playlist::Genre, m_track->genre()->prettyName() );
    }
    if ((isVisible(Playlist::Year)) && (m_track->year()))
    {
        text += HTMLLine( Playlist::Year, m_track->year()->name().toInt() );
    }
    if (isVisible(Playlist::Comment))
    {
        text += HTMLLine( Playlist::Comment, m_track->comment() );
    }
    if (isVisible(Playlist::Score))
    {
        text += HTMLLine( Playlist::Score, QString::number( static_cast<int>( m_track->score() ) ), true );
    }
    if (isVisible(Playlist::Rating))
    {
        text += HTMLLine( Playlist::Rating, QString::number( static_cast<double>(m_track->rating())/2.0 ), true );
    }
    if (isVisible(Playlist::PlayCount))
    {
        text += HTMLLine( Playlist::PlayCount, m_track->playCount(), true );
    }

    if (text.isEmpty())
    {
        text = QString( i18n( "No extra information available" ) );
    }
    else
    {       
        text = QString("<table>"+ text +"</table>");
    }
    showToolTip(image, text);
    
}

void ToolTipManager::showToolTip(const QIcon& icon, const QString& text)
{
    if (QApplication::mouseButtons() & Qt::LeftButton) {
        return;
    }

    KToolTipItem* tip = new KToolTipItem(icon, text);

    KStyleOptionToolTip option;
    // TODO: get option content from KToolTip or add KToolTip::sizeHint() method
    option.direction      = QApplication::layoutDirection();
    option.fontMetrics    = QFontMetrics(QToolTip::font());
    option.activeCorner   = KStyleOptionToolTip::TopLeftCorner;
    option.palette        = QToolTip::palette();
    option.font           = QToolTip::font();
    option.rect           = QRect();
    option.state          = QStyle::State_None;
    option.decorationSize = QSize(32, 32);

    const QSize size = g_delegate->sizeHint(option, *tip);
    const QRect desktop = QApplication::desktop()->screenGeometry(m_itemRect.bottomRight());

    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown in the bottom right corner.
    // If the tooltip content exceeds the desktop borders, it must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    const bool hasRoomToLeft  = (m_itemRect.left()   - size.width()  >= desktop.left());
    const bool hasRoomToRight = (m_itemRect.right()  + size.width()  <= desktop.right());
    const bool hasRoomAbove   = (m_itemRect.top()    - size.height() >= desktop.top());
    const bool hasRoomBelow   = (m_itemRect.bottom() + size.height() <= desktop.bottom());
    if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
        delete tip;
        tip = 0;
        return;
    }

    int x = 0;
    int y = 0;
    if (hasRoomBelow || hasRoomAbove) {
        x = QCursor::pos().x() + 16; // TODO: use mouse pointer width instead of the magic value of 16
        if (x + size.width() >= desktop.right()) {
            x = desktop.right() - size.width();
        }
        y = hasRoomBelow ? m_itemRect.bottom() : m_itemRect.top() - size.height();
    } else {
        Q_ASSERT(hasRoomToLeft || hasRoomToRight);
        x = hasRoomToRight ? m_itemRect.right() : m_itemRect.left() - size.width();

        // Put the tooltip at the bottom of the screen. The x-coordinate has already
        // been adjusted, so that no overlapping with m_itemRect occurs.
        y = desktop.bottom() - size.height();
    }

    // the ownership of tip is transferred to KToolTip
    KToolTip::showTip(QPoint(x, y), tip);
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The colunm used to display the icon
* @param value The QString value to be shown
* @return The line to be shown or an empty QString if the value is null
*/
QString ToolTipManager::HTMLLine( const Playlist::Column& column, const QString& value, bool force )
{
    if ( (!value.isEmpty()) || (force) )
    {
        QString line = QString();
        line += "<tr><td align=\"right\">";
        line += "<img src=\""+KIconLoader::global()->iconPath( Playlist::iconNames[column] , -16)+"\" />";
        line += "</td><td align=\"left\">";
        line += breakLongLinesHTML( value );
        line += "</td></tr>";
        return line;
    }
    else
        return QString();
}

/**
* Prepares a row for the playlist tooltips consisting of an icon representing
* an mp3 tag and its value
* @param column The colunm used to display the icon
* @param value The integer value to be shown
* @return The line to be shown or an empty QString if the value is 0
*/
QString ToolTipManager::HTMLLine( const Playlist::Column& column, const int value, bool force )
{
    if ( (value != 0) || (force) )
    {
        return HTMLLine( column, QString::number( value ) );
    }
    else
        return QString();
}

QString ToolTipManager::breakLongLinesHTML(const QString& text)
{
    // Now let's break up long lines so that the tooltip doesn't become hideously large

    // The size of the normal, standard line
    const int lnSize = 50;
    if (text.size() <= lnSize)
    {
        // If the text is not too long, return it as it is
        return text;
    }
    else
    {
        QString textInLines;
        
        QStringList words = text.trimmed().split(' ');
        int lineLength = 0;
        while(words.size() > 0)
        {
            QString word = words.first();
            // Let's check if the next word makes the current line too long.
            if (lineLength + word.size() + 1 > lnSize)
            {
                if (lineLength > 0)
                {
                    textInLines += "<br/>";
                }
                lineLength = 0;
                // Let's check if the next word is not too long for the new line to contain
                // If it is, cut it
                while (word.size() > lnSize)
                {
                    QString wordPart = word;
                    wordPart.resize(lnSize);
                    word.remove(0,lnSize);
                    textInLines += wordPart + "<br/>";
                }
            }
            textInLines += word + " ";
            lineLength += word.size() + 1;
            words.removeFirst();
        }
        return textInLines.trimmed();
    }
}

/**
 * Exclude the field from being shown on the tooltip because it's already on the playlist
 * @param column The column to be excluded
 */
void ToolTipManager::excludeField(const Playlist::Column& column, bool single)
{
    if (single)
    {
        m_excludes_single[column] = true;
        // The "Track with track number", as it contains two pieces of data,
        // deserves a special treatment
        if (column == Playlist::TitleWithTrackNum)
        {
            m_excludes_single[Playlist::Title] = true;
            m_excludes_single[Playlist::TrackNumber] = true;
        }
    }
    else
    {
        m_excludes[column] = true;
        if (column == Playlist::TitleWithTrackNum)
        {
            m_excludes[Playlist::Title] = true;
            m_excludes[Playlist::TrackNumber] = true;
        }
    }
}

/**
* Exclude the album cover from being shown on the tooltip because it's already on the playlist
* @param single If ON, the item is excluded from the tooltip for a Single item (not Head or Body)
*/
void ToolTipManager::excludeCover( bool single )
{
    excludeField(Playlist::CoverImage, single);
}

/**
* Cancel all exclusions
*/
void ToolTipManager::cancelExclusions()
{
    for (int i = 0 ;i < Playlist::NUM_COLUMNS; i++ )
    {
        m_excludes[i] = false;
        m_excludes_single[i] = false;
    }

}

/**
* Returns TRUE if a data of a column should be shown
* Makes it easier to determine considering the Single items don't share
* configuration with Head or Body items
* @param column The column in question
* @return TRUE if the line should be shown
*/
bool ToolTipManager::isVisible( const Playlist::Column& column )
{
    if (m_singleItem)
    {
        return !m_excludes_single[column];
    }
    else
    {
        return !m_excludes[column];
    }
}


#include "ToolTipManager.moc"
