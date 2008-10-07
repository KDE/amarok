/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>
 *                      : (C) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 *                      : (C) 2008 Soren Harward <stharward@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **************************************************************************/

#define DEBUG_PREFIX "Playlist::PrettyItemDelegate"

#include "Debug.h"
#include "PrettyItemDelegate.h"
#include "SvgHandler.h"
#include "SvgTinter.h"
#include "meta/Meta.h"
#include "meta/SourceInfoCapability.h"
#include "playlist/GroupingProxy.h"
#include "playlist/PlaylistModel.h"

#include <QFontMetricsF>
#include <QPainter>

const qreal Playlist::PrettyItemDelegate::ALBUM_WIDTH = 50.0;
const qreal Playlist::PrettyItemDelegate::SINGLE_TRACK_ALBUM_WIDTH = 40.0;
const qreal Playlist::PrettyItemDelegate::MARGIN = 2.0;
const qreal Playlist::PrettyItemDelegate::MARGINH = 6.0;
const qreal Playlist::PrettyItemDelegate::MARGINBODY = 1.0;
const qreal Playlist::PrettyItemDelegate::PADDING = 1.0;
QFontMetricsF* Playlist::PrettyItemDelegate::s_nfm = 0; // normal font metric
QFontMetricsF* Playlist::PrettyItemDelegate::s_bfm = 0; // bold font metric
QFontMetricsF* Playlist::PrettyItemDelegate::s_ifm = 0; // italic font metric

Playlist::PrettyItemDelegate::PrettyItemDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    DEBUG_BLOCK

    QFont font;

    if (!s_nfm) {
        s_nfm = new QFontMetricsF(font);

        if (!s_bfm) {
            font.setBold(true);
            s_bfm = new QFontMetricsF(font);
            font.setBold(false);
        }
        if (!s_ifm) {
            font.setItalic(true);
            s_ifm = new QFontMetricsF(font);
            font.setItalic(false);
        }
    }
}

Playlist::PrettyItemDelegate::~PrettyItemDelegate() { }

QSize
Playlist::PrettyItemDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex& index) const {

    /* Qt's ItemViews are horrible about calling sizeHint() and paint() for
     * invalid indexes when the model has been reset.  Ideally, this safety
     * check shouldn't be necessary, but without it this function will crash
     * when handed an invalid index. -- stharward */

    if (!Model::instance()->rowExists(index.row()))
        return QSize();

    int height = 0;

    int groupMode = index.data(GroupRole).toInt();
    switch (groupMode) {
        case Head:
            height = static_cast<int>(MARGIN + qMax(ALBUM_WIDTH, s_bfm->height()*2+PADDING) + 3*PADDING + s_nfm->height() + MARGINBODY);
            break;
        case Body:
            height = static_cast<int>(s_nfm->height() + 2*PADDING + 2*MARGINBODY);
            break;
        case Tail:
            height = static_cast<int>(MARGINBODY + s_nfm->height() + 2*PADDING + MARGIN);
            break;
        case None:
        default:
            height = static_cast<int>(qMax(SINGLE_TRACK_ALBUM_WIDTH, s_nfm->height()*2+4*PADDING) + 2*MARGIN);
            break;
    }

    return QSize(120, height);
}

void
Playlist::PrettyItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {

    /* see note in sizeHint() about this safety check */

    if (!Model::instance()->rowExists(index.row()))
        return;

    painter->save();
    painter->translate(option.rect.topLeft());

    if ((index.row() % 2) == 0)
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "track", (int)option.rect.width(), (int)option.rect.height(), "track" ) );
    else
        painter->drawPixmap( 0, 0, The::svgHandler()->renderSvgWithDividers( "alt_track", (int)option.rect.width(), (int)option.rect.height(), "alt_track" ) );

    // call paint method based on type
    int groupMode = index.data(GroupRole).toInt();
    if ( groupMode == None )
        paintSingleTrack( painter, option, index );
    else if ( groupMode == Head )
        paintHead( painter, option, index );
    else if ( groupMode == Body )
        paintBody( painter, option, index );
    else if ( groupMode == Tail )
        paintBody( painter, option, index );
    /*else if ( groupMode == Head_Collapsed )
        paintCollapsedHead( painter, option, index );
    else if ( m_groupMode == Collapsed )
        paintCollapsed( );*/
    else
        QStyledItemDelegate::paint(painter, option, index);

    painter->restore();
}

bool
Playlist::PrettyItemDelegate::insideItemHeader(const QPoint& pt, const QRect& rect) {
    QRect headerBounds = rect.adjusted((int)MARGINH,
                                       (int)MARGIN,
                                       (int)(-MARGINH),
                                       (int)(-MARGIN - 2*PADDING - s_nfm->height()));
    return headerBounds.contains(pt);
}

void
Playlist::PrettyItemDelegate::paintSingleTrack(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QRectF trackRect(option.rect);

    Meta::TrackPtr track = index.data(TrackRole).value<Meta::TrackPtr>();

    //paint cover
    QPixmap albumPixmap;
    if (track->album())
        albumPixmap = track->album()->imageWithBorder(int(SINGLE_TRACK_ALBUM_WIDTH), 3);

    //offset cover if non square
    QPointF offset = centerImage(albumPixmap, imageLocation());
    QRectF imageRect( imageLocationSingleTrack().x() + offset.x(),
                      imageLocationSingleTrack().y() + offset.y(),
                      imageLocationSingleTrack().width() - offset.x() * 2,
                      imageLocationSingleTrack().height() - offset.y() * 2 );

    painter->drawPixmap(imageRect, albumPixmap, QRectF(albumPixmap.rect()));

    //check if there is a emblem to display
    //does this track have the SourceInfoCapability?
    Meta::SourceInfoCapability *sic = track->as<Meta::SourceInfoCapability>();
    if (sic) {
        //is the source defined
        QString source = sic->sourceName();
        if( !source.isEmpty() )
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );

        delete sic;
    }

    // Set up the text areas
    qreal leftside = MARGINH + SINGLE_TRACK_ALBUM_WIDTH + 3*PADDING;
    qreal boxheight = (trackRect.height() - 3*MARGIN) / 2.0;
    qreal textwidth = trackRect.width() - leftside - MARGINH;

    QPointF topLeft(leftside, MARGIN);
    QRectF topLine(leftside, MARGIN, textwidth, boxheight); // box that surrounds the entire top line of text
    QRectF bottomLine(leftside, MARGIN + boxheight + MARGIN, textwidth, boxheight); // box that surrounds the entire bottom line of text

    // top right: track time
    QString timeString;
    if (track->length() > 3600)
        timeString = QTime().addSecs(track->length()).toString("h:mm:ss");
    else
        timeString = QTime().addSecs(track->length()).toString("m:ss");
    QSizeF timeStringSize(s_nfm->size(Qt::TextSingleLine, timeString));
    timeStringSize.setHeight(boxheight);
    QPointF textLoc(trackRect.width() - MARGINH - timeStringSize.width() - PADDING, MARGIN);
    QRectF timeTextBox(textLoc, timeStringSize);
    timeTextBox = timeTextBox.adjusted(0,PADDING,0,-PADDING);

    // top left: track name
    QRectF titleTextBox = topLine.adjusted(PADDING, PADDING, -2*PADDING - timeStringSize.width(), -PADDING);
    QString titleString = s_nfm->elidedText(track->prettyName(), Qt::ElideRight, (int)titleTextBox.width());

    QString rawArtistString = (track->album() == Meta::ArtistPtr()) ? QString() : track->artist()->prettyName();
    QString rawAlbumString = (track->album() == Meta::AlbumPtr()) ? QString() : track->album()->prettyName();

    // figure out widths for artist and album boxes
    qreal artistNameWidth = s_nfm->size(Qt::TextSingleLine, rawArtistString).width();
    qreal albumNameWidth;
    if (artistNameWidth <= (textwidth-5.0)/2.0) {
        albumNameWidth = textwidth - (artistNameWidth + 5.0);
    } else {
        albumNameWidth = s_nfm->size(Qt::TextSingleLine, rawAlbumString).width();
        if (albumNameWidth <= (textwidth-5.0)/2.0) {
            artistNameWidth = textwidth - (albumNameWidth + 5.0);
        } else {
            artistNameWidth = (textwidth-5.0)/2.0;
            albumNameWidth = (textwidth-5.0)/2.0;
        }
    }

    // bottom left: artist name
    QPointF artistLoc(bottomLine.topLeft());
    artistLoc.rx() += PADDING;
    QRectF artistTextBox(artistLoc, QSizeF(artistNameWidth,boxheight));
    artistTextBox = artistTextBox.adjusted(0, PADDING, 0, -PADDING);
    QString artistString = s_nfm->elidedText(rawArtistString, Qt::ElideRight, (int)artistTextBox.width());

    // bottom right: album name
    QPointF albumLoc(bottomLine.topLeft());
    albumLoc.rx() += (3.0 + artistNameWidth);
    QRectF albumTextBox(albumLoc, QSizeF(albumNameWidth,boxheight));
    albumTextBox = albumTextBox.adjusted(0, PADDING, 0, -PADDING);
    QString albumString = s_nfm->elidedText(rawAlbumString, Qt::ElideRight, (int)albumTextBox.width());

    // draw the "current track" highlight underneath the text
    if (index.data(ActiveTrackRole).toBool()) {
        painter->drawPixmap( (int)topLine.x(),(int)topLine.y(),
                             The::svgHandler()->renderSvg(
                                        "active_overlay",
                                        (int)bottomLine.width(), (int)bottomLine.height(),
                                        "active_overlay"
                                      )
                           );
    }

    // render text in here
    //setTextColor(index.data(ActiveTrackRole).toBool());
    painter->drawText(titleTextBox, Qt::AlignLeft | Qt::AlignVCenter, titleString);
    painter->drawText(timeTextBox, Qt::AlignLeft | Qt::AlignVCenter, timeString);
    painter->drawText(artistTextBox, Qt::AlignLeft | Qt::AlignVCenter, artistString);
    painter->drawText(albumTextBox, Qt::AlignRight | Qt::AlignVCenter, albumString);

    //set selection marker if needed
    if (option.state & QStyle::State_Selected) {
        painter->drawPixmap( (int)topLine.x(),(int)topLine.y(),
                             The::svgHandler()->renderSvg(
                                        "selection",
                                        (int)bottomLine.width(), (int)bottomLine.height(),
                                        "selection"
                                      )
                           );
    }
}

void
Playlist::PrettyItemDelegate::paintHead(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QRectF trackRect(option.rect);

    Meta::TrackPtr track = index.data(TrackRole).value<Meta::TrackPtr>();

    //paint cover
    QPixmap albumPixmap;
    if (track->album())
        albumPixmap = track->album()->imageWithBorder( int( ALBUM_WIDTH ), 3 );

    //offset cover if non square
    QPointF offset = centerImage( albumPixmap, imageLocation() );
    QRectF imageRect( imageLocation().x() + offset.x(),
                      imageLocation().y() + offset.y(),
                      imageLocation().width() - offset.x() * 2,
                      imageLocation().height() - offset.y() * 2 );

    painter->drawPixmap( imageRect, albumPixmap, QRectF( albumPixmap.rect() ) );

    //check if there is a emblem to display
    //does this track have the SourceInfoCapability?
    Meta::SourceInfoCapability *sic = track->as<Meta::SourceInfoCapability>();
    if (sic) {
        //is the source defined
        QString source = sic->sourceName();
        if( !source.isEmpty() )
            painter->drawPixmap( QRectF( imageLocation().x(), imageLocation().y() , 16, 16 ), sic->emblem(), QRectF( 0, 0 , 16, 16 ) );

        delete sic;
    }

    qreal headheight = MARGIN + qMax(ALBUM_WIDTH, s_bfm->height()*2+PADDING) + PADDING;

    // Set up the text areas
    qreal leftside = MARGINH + ALBUM_WIDTH + 3*PADDING;
    qreal boxheight = (headheight - PADDING) / 2.0;
    qreal textwidth = trackRect.width() - leftside - MARGINH;

    QPointF topLeft(leftside, MARGIN);
    QRectF topLine(leftside, MARGIN, textwidth, boxheight); // box that surrounds the entire top line of text
    QRectF bottomLine(leftside, MARGIN + boxheight + MARGIN, textwidth, boxheight); // box that surrounds the entire bottom line of text

    painter->save(); // before changing to bold text

    QFont font;
    font.setBold(true);
    painter->setFont(font);

    // top line: album
    QRectF albumTextBox = topLine.adjusted(PADDING, PADDING, -PADDING, -PADDING);
    QString rawAlbumString = (track->album() == Meta::AlbumPtr()) ? QString() : track->album()->prettyName();
    QString albumString = s_bfm->elidedText(rawAlbumString, Qt::ElideRight, (int)albumTextBox.width());
    painter->drawText(albumTextBox, Qt::AlignCenter, albumString);

    // bottom line: artist
    QRectF artistTextBox = bottomLine.adjusted(PADDING, PADDING, -PADDING, -PADDING);
    QString rawArtistString = (track->album() == Meta::ArtistPtr()) ? QString() : track->artist()->prettyName();
    QString artistString = s_bfm->elidedText(rawArtistString, Qt::ElideRight, (int)artistTextBox.width());
    painter->drawText(artistTextBox, Qt::AlignCenter, artistString);

    painter->restore(); // change back from bold text

    // for track number, name, and time
    QRectF line(QPointF(MARGINH, headheight + MARGIN), QPointF(trackRect.width() - MARGINH, trackRect.height() - MARGINBODY));

    // draw the "current track" highlight underneath the text
    if (index.data(ActiveTrackRole).toBool()) {
        painter->drawPixmap( (int)line.x(),(int)line.y(),
                             The::svgHandler()->renderSvg(
                                        "active_overlay",
                                        (int)line.width(), (int)line.height(),
                                        "active_overlay"
                                      )
                           );
    }

    // right: track time
    QString timeString;
    if (track->length() > 3600)
        timeString = QTime().addSecs(track->length()).toString("h:mm:ss");
    else
        timeString = QTime().addSecs(track->length()).toString("m:ss");
    QSizeF timeStringSize(s_nfm->size(Qt::TextSingleLine, timeString));
    timeStringSize.setHeight(line.height());
    QPointF textLoc(trackRect.width() - MARGINH - timeStringSize.width() - PADDING, line.y());
    QRectF timeTextBox(textLoc, timeStringSize);
    timeTextBox = timeTextBox.adjusted(0,PADDING,0,-PADDING);

    // left: track number and name
    QRectF textBox = line.adjusted(PADDING,PADDING,-2*PADDING -timeStringSize.width(),-PADDING);


    QString trackString;
    QString trackName = track->prettyName();
    if ( track->trackNumber() > 0 ) {
        QString trackNumber = QString::number( track->trackNumber() );
        trackString = s_nfm->elidedText(QString(trackNumber + " - " + trackName), Qt::ElideRight, (int)textBox.width());
    } else
        trackString = s_nfm->elidedText( QString( trackName ), Qt::ElideRight, (int)textBox.width() );

    // render text in here
    //setTextColor(index.data(ActiveTrackRole).toBool());
    painter->drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter, trackString);
    painter->drawText(timeTextBox, Qt::AlignRight | Qt::AlignVCenter, timeString);

    //set selection marker if needed
    if (option.state & QStyle::State_Selected) {
        painter->drawPixmap( (int)line.x(),(int)line.y(),
                             The::svgHandler()->renderSvg(
                                        "selection",
                                        (int)line.width(), (int)line.height(),
                                        "selection"
                                      )
                           );
    }
}
void
Playlist::PrettyItemDelegate::paintBody(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
    QRectF trackRect(option.rect);

    QRectF line(MARGINH, MARGINBODY, trackRect.width() - (2*MARGINH), trackRect.height() - (2*MARGINBODY));

    // draw the "current track" highlight underneath the text
    if (index.data(ActiveTrackRole).toBool()) {
        painter->drawPixmap( (int)line.x(),(int)line.y(),
                             The::svgHandler()->renderSvg(
                                        "active_overlay",
                                        (int)line.width(), (int)line.height(),
                                        "active_overlay"
                                      )
                           );
    }

    Meta::TrackPtr track = index.data(TrackRole).value<Meta::TrackPtr>();

    // right: track time
    QString timeString;
    if (track->length() > 3600)
        timeString = QTime().addSecs(track->length()).toString("h:mm:ss");
    else
        timeString = QTime().addSecs(track->length()).toString("m:ss");
    QSizeF timeStringSize(s_nfm->size(Qt::TextSingleLine, timeString));
    timeStringSize.setHeight(line.height());
    QPointF textLoc(trackRect.width() - MARGINH - timeStringSize.width() - PADDING, MARGIN);
    QRectF timeTextBox(textLoc, timeStringSize);
    timeTextBox = timeTextBox.adjusted(0,PADDING,0,-PADDING);

    // left: track number and name
    QRectF textBox = line.adjusted(PADDING,PADDING,-2*PADDING-timeStringSize.width(),-PADDING);

    QString trackString;
    QString trackName = track->prettyName();
    if ( track->trackNumber() > 0 ) {
        QString trackNumber = QString::number( track->trackNumber() );
        trackString = s_nfm->elidedText(QString(trackNumber + " - " + trackName), Qt::ElideRight, (int)textBox.width());
    } else
        trackString = s_nfm->elidedText( QString( trackName ), Qt::ElideRight, (int)textBox.width() );

    // render text in here
    //setTextColor(index.data(ActiveTrackRole).toBool());
    painter->drawText(textBox, Qt::AlignLeft | Qt::AlignVCenter, trackString);
    painter->drawText(timeTextBox, Qt::AlignRight | Qt::AlignVCenter, timeString);

    //set selection marker if needed
    if (option.state & QStyle::State_Selected) {
        painter->drawPixmap( (int)line.x(),(int)line.y(),
                             The::svgHandler()->renderSvg(
                                        "selection",
                                        (int)line.width(), (int)line.height(),
                                        "selection"
                                      )
                           );
    }
}



QPointF
Playlist::PrettyItemDelegate::centerImage(const QPixmap& pixmap, const QRectF& rect) const
{
    qreal pixmapRatio = (qreal)pixmap.width() / (qreal)pixmap.height();

    qreal moveByX = 0.0;
    qreal moveByY = 0.0;

    if (pixmapRatio >= 1)
        moveByY = (rect.height() - (rect.width() / pixmapRatio))/2.0;
    else
        moveByX = (rect.width() - (rect.height() * pixmapRatio))/2.0;

    return QPointF(moveByX, moveByY);

}

#if 0
void
Playlist::PrettyItemDelegate::setTextColor(const QModelIndex& index) const {
    int state = index.data(StateRole).toInt();

    if (index.data(ActiveTrackRole).toBool()) {
        m_items->bottomLeftText->setDefaultTextColor(App::instance()->palette().brightText().color());
        m_items->bottomRightText->setDefaultTextColor(App::instance()->palette().brightText().color());
    } else {
        QColor textColor;
        switch (state) {
            // TODO: what should these be really ?
            case Item::NewlyAdded:
                textColor = App::instance()->palette().link().color();
                m_items->bottomLeftText->setDefaultTextColor(textColor);
                m_items->bottomRightText->setDefaultTextColor(textColor);
                break;

            case Item::DynamicPlayed:
                textColor = App::instance()->palette().brush(QPalette::Disabled, QPalette::ButtonText).color();
                m_items->bottomLeftText->setDefaultTextColor(textColor);
                m_items->bottomRightText->setDefaultTextColor(textColor);
                break;

            case Item::Normal:
            default:
                textColor = App::instance()->palette().text().color();
                m_items->bottomLeftText->setDefaultTextColor(textColor);
                m_items->bottomRightText->setDefaultTextColor(textColor);
        }
    }
}
#endif
