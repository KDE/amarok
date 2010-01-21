/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *   Copyright (C) 2008 Oleksandr Khayrullin <saniokh@gmail.com>               *
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

#include "amarokconfig.h"
#include "tooltipmanager.h"

#include "amaroktooltip.h"
#include "meta/Meta.h"
#include "playlist/proxymodels/GroupingProxy.h"

#include <tooltips/ktooltip.h>

#include <QAbstractItemView>
#include <QApplication>
#include <QDesktopWidget>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>

const int ICON_WIDTH = 128;
const int ICON_HEIGHT = 128;
const int PREVIEW_DELAY = 250;

ToolTipManager::ToolTipManager(QAbstractItemView* parent) :
    QObject(parent),
    m_view(parent),
    m_timer(0),
    m_previewTimer(0),
    m_waitOnPreviewTimer(0),
    m_itemRect(),
    m_generatingPreview(false),
    m_hasDefaultIcon(false),
    m_previewPixmap()
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

    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    connect(m_previewTimer, SIGNAL(timeout()),
            this, SLOT(startPreviewJob()));

    m_waitOnPreviewTimer = new QTimer(this);
    m_waitOnPreviewTimer->setSingleShot(true);
    connect(m_waitOnPreviewTimer, SIGNAL(timeout()),
            this, SLOT(prepareToolTip()));

    // When the mousewheel is used, the items don't get a hovered indication
    // (Qt-issue #200665). To assure that the tooltip still gets hidden,
    // the scrollbars are observed.
    connect(parent->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));

    m_view->viewport()->installEventFilter(this);
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
    if ((AmarokConfig::enableToolTips()) && (track) && !(QApplication::mouseButtons() & Qt::LeftButton)) {
        m_track = track;
        m_waitOnPreviewTimer->stop();
        KToolTip::hideTip();

        m_itemRect = m_view->visualRect(index);
        const QPoint pos = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);
        
        // only start the previewJob when the mouse has been over this item for 200 milliseconds,
        // this prevents a lot of useless preview jobs when passing rapidly over a lot of items
        m_previewTimer->start(200);
        m_previewPixmap = QPixmap();
        m_hasDefaultIcon = false;

        m_timer->start(500);
    } else {
        hideToolTip();
    }
}

void ToolTipManager::hideToolTip()
{
    m_timer->stop();
    m_previewTimer->stop();
    //m_waitOnPreviewTimer->stop();
    KToolTip::hideTip();
}

void ToolTipManager::prepareToolTip()
{
/*    if (m_generatingPreview) {
        m_waitOnPreviewTimer->start(250);
    }*/

    QPixmap image;
    if (m_track->album()->hasImage())
        image = m_track->album()->image();
    else
        image = QPixmap();

    QString text = QString();
    if (m_track->name() != "")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Title")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->prettyName())+"</td></tr>";
    }
    if (m_track->artist()->name() != "")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Artist")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->artist()->prettyName())+"</td></tr>";
    }
    if (m_track->composer()->name() != "")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Composer")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->composer()->prettyName())+"</td></tr>";
    }
    if (m_track->album()->name() != "")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Album")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->album()->prettyName())+"</td></tr>";
    }
    if (m_track->discNumber() != 0)
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Disk Number")+"</b>:</td><td align=\"left\">"+QString::number(m_track->discNumber())+"</td></tr>";
    }
    if (m_track->genre()->name() != "")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Genre")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->genre()->prettyName())+"</td></tr>";
    }
    if (m_track->trackNumber() != 0)
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Track")+"</b>:</td><td align=\"left\">"+QString::number(m_track->trackNumber())+"</td></tr>";
    }
    if (m_track->year()->name() != "0")
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Year")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->year()->prettyName())+"</td></tr>";
    }
    if ((m_track->comment() != ""))
    {
        text += "<tr><td align=\"right\"><b>"+i18n("Comment")+"</b>:</td><td align=\"left\">"+breakLongLinesHTML(m_track->comment())+"</td></tr>";
    }
    //text += "<tr><td align=\"right\"><b>"+i18n("Location")+"</b>:</td><td align=\"left\"><a href=\""+m_track->prettyUrl()+"\">"+m_track->prettyUrl()+"</a></td></tr>";
    if (text == "")
    {
        text = QString(i18n("No information available"));
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
#include "tooltipmanager.moc"
