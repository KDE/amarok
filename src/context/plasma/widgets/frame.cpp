/*
 *   Copyright 2008 Marco Martin <notmart@gmail.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "frame.h"

//Qt
#include <QPainter>
#include <QGraphicsSceneResizeEvent>
#include <QWidget>
#include <QDir>
#include <QApplication>

//KDE
#include <KMimeType>

//Plasma
#include "plasma/theme.h"
#include "plasma/framesvg.h"

namespace Plasma
{

class FramePrivate
{
public:
    FramePrivate(Frame *parent)
        : q(parent),
          svg(0),
          image(0),
          pixmap(0)
    {
    }

    ~FramePrivate()
    {
        delete pixmap;
    }

    void syncBorders();

    Frame *q;
    FrameSvg *svg;
    Frame::Shadow shadow;
    QString text;
    QString styleSheet;
    QString imagePath;
    QString absImagePath;
    Svg *image;
    QPixmap *pixmap;
};

void FramePrivate::syncBorders()
{
    //set margins from the normal element
    qreal left, top, right, bottom;

    svg->getMargins(left, top, right, bottom);

    if (!text.isNull()) {
        QFontMetricsF fm(QApplication::font());
        top += fm.height();
    }

    q->setContentsMargins(left, top, right, bottom);
}

Frame::Frame(QGraphicsWidget *parent)
    : QGraphicsWidget(parent),
      d(new FramePrivate(this))
{
    d->svg = new Plasma::FrameSvg(this);
    d->svg->setImagePath("widgets/frame");
    d->svg->setElementPrefix("plain");
    d->syncBorders();

    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), SLOT(syncBorders()));
}

Frame::~Frame()
{
    delete d;
}

void Frame::setFrameShadow(Shadow shadow)
{
    d->shadow = shadow;

    switch (d->shadow) {
    case Raised:
        d->svg->setElementPrefix("raised");
        break;
    case Sunken:
        d->svg->setElementPrefix("sunken");
        break;
    case Plain:
    default:
        d->svg->setElementPrefix("plain");
        break;
    }

    d->syncBorders();
}

Frame::Shadow Frame::frameShadow() const
{
    return d->shadow;
}

void Frame::setText(QString text)
{
    d->text = text;
    d->syncBorders();
}

QString Frame::text() const
{
    return d->text;
}

void Frame::setImage(const QString &path)
{
    if (d->imagePath == path) {
        return;
    }

    delete d->image;
    d->image = 0;
    d->imagePath = path;
    delete d->pixmap;
    d->pixmap = 0;

    bool absolutePath = !path.isEmpty() &&
                        #ifdef Q_WS_WIN
                            !QDir::isRelativePath(path)
                        #else
                            (path[0] == '/' || path.startsWith(":/"))
                        #endif
        ;

    if (absolutePath) {
        d->absImagePath = path;
    } else {
        //TODO: package support
        d->absImagePath = Theme::defaultTheme()->imagePath(path);
    }

    if (path.isEmpty()) {
        return;
    }

    KMimeType::Ptr mime = KMimeType::findByPath(d->absImagePath);

    if (!mime->is("image/svg+xml") && !mime->is("application/x-gzip")) {
        d->pixmap = new QPixmap(d->absImagePath);
    } else {
        d->image = new Plasma::Svg(this);
        d->image->setImagePath(path);
    }
}

QString Frame::image() const
{
    return d->imagePath;
}

void Frame::setStyleSheet(const QString &styleSheet)
{
    //TODO: implement stylesheets painting
    d->styleSheet = styleSheet;
}

QString Frame::styleSheet() const
{
    return d->styleSheet;
}

QWidget *Frame::nativeWidget() const
{
    return 0;
}

void Frame::paint(QPainter *painter,
                   const QStyleOptionGraphicsItem *option,
                   QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    d->svg->paintFrame(painter);

    if (!d->text.isNull()) {
        QFontMetricsF fm(QApplication::font());
        QRectF textRect = d->svg->contentsRect();
        textRect.setHeight(fm.height());
        painter->setPen(Plasma::Theme::defaultTheme()->color(Theme::TextColor));
        painter->drawText(textRect, Qt::AlignHCenter|Qt::AlignTop, d->text);
    }

    if (!d->imagePath.isNull()) {
        if (d->pixmap && !d->pixmap->isNull()) {
            painter->drawPixmap(contentsRect(), *d->pixmap, d->pixmap->rect());
        } else if (d->image) {
            d->image->paint(painter, contentsRect());
        }
    }
}

void Frame::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->svg->resizeFrame(event->newSize());

    if (d->image) {
        d->image->resize(contentsRect().size());
    }
}

QSizeF Frame::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    QSizeF hint = QGraphicsWidget::sizeHint(which, constraint);

    if (!d->image && !layout()) {
        QFontMetricsF fm(QApplication::font());

        qreal left, top, right, bottom;
        d->svg->getMargins(left, top, right, bottom);

        hint.setHeight(fm.height() + top + bottom);
    }

    return hint;
}

} // namespace Plasma

#include <frame.moc>

