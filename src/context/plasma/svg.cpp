/*
 *   Copyright 2006-2007 Aaron Seigo <aseigo@kde.org>
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

#include "svg.h"

#include <QDir>
#include <QMatrix>
#include <QPainter>
#include <QSharedData>

#include <KColorScheme>
#include <KConfigGroup>
#include <KDebug>
#include <KIconEffect>
#include <KGlobalSettings>
#include <KSharedPtr>
#include <KSvgRenderer>

#include "theme.h"

namespace Plasma
{

class SharedSvgRenderer : public KSvgRenderer, public QSharedData
{
    public:
        typedef KSharedPtr<SharedSvgRenderer> Ptr;

        SharedSvgRenderer(QObject *parent = 0)
            : KSvgRenderer(parent)
        {}

        SharedSvgRenderer(const QString &filename, QObject *parent = 0)
            : KSvgRenderer(filename, parent)
        {}

        SharedSvgRenderer(const QByteArray &contents, QObject *parent = 0)
            : KSvgRenderer(contents, parent)
        {}

        ~SharedSvgRenderer()
        {
            //kDebug() << "leaving this world for a better one.";
        }
};

class SvgPrivate
{
    public:
        SvgPrivate(Svg *svg)
            : q(svg),
              renderer(0),
              multipleImages(false),
              themed(false),
              applyColors(false)
        {
        }

        ~SvgPrivate()
        {
            eraseRenderer();
        }

        QString cacheId(const QString &elementId)
        {
            if (size.isValid() && size != naturalSize) {
                return QString("%3_%2_%1").arg(int(size.height()))
                                        .arg(int(size.width()))
                                        .arg(elementId);
            } else {
                return QString("%2_%1").arg("Natural")
                                        .arg(elementId);
            }
        }

        bool setImagePath(const QString &imagePath, Svg *q)
        {
            bool isThemed = !QDir::isAbsolutePath(imagePath);

            // lets check to see if we're already set to this file
            if (isThemed == themed &&
                ((themed && themePath == imagePath) ||
                 (!themed && path == imagePath))) {
                return false;
            }

            // if we don't have any path right now and are going to set one,
            // then lets not schedule a repaint because we are just initializing!
            bool updateNeeded = true; //!path.isEmpty() || !themePath.isEmpty();

            if (themed) {
                QObject::disconnect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),
                                    q, SLOT(themeChanged()));
                QObject::disconnect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                    q, SLOT(colorsChanged()));
            }

            themed = isThemed;
            path.clear();
            themePath.clear();

            if (themed) {
                themePath = imagePath;
                QObject::connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()),
                                 q, SLOT(themeChanged()));

                // check if svg wants colorscheme applied
                checkApplyColorHint();
                if (applyColors && !Theme::defaultTheme()->colorScheme()) {
                    QObject::connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                     q, SLOT(colorsChanged()));
                }
            } else if (QFile::exists(imagePath)) {
                path = imagePath;
            } else {
                kDebug() << "file '" << path << "' does not exist!";
            }

            return updateNeeded;
        }

        QPixmap findInCache(const QString &elementId, const QSizeF &s = QSizeF())
        {
            QSize size;
            if (elementId.isEmpty() || (multipleImages && s.isValid())) {
                size = s.toSize();
            } else {
                size = elementRect(elementId).size().toSize();
            }

            if (!size.isValid()) {
                return QPixmap();
            }

            QString id = QString::fromLatin1("%3_%2_%1_").
                         arg(size.width()).arg(size.height()).arg(path);

            if (!elementId.isEmpty()) {
                id.append(elementId);
            }

            //kDebug() << "id is " << id;

            Theme *theme = Theme::defaultTheme();
            QPixmap p;

            if (theme->findInCache(id, p)) {
                //kDebug() << "found cached version of " << id << p.size();
                return p;
            } else {
                //kDebug() << "didn't find cached version of " << id << ", so re-rendering";
            }

            //kDebug() << "size for " << elementId << " is " << s;
            // we have to re-render this puppy

            p = QPixmap(size);

            p.fill(Qt::transparent);
            QPainter renderPainter(&p);

            createRenderer();
            if (elementId.isEmpty()) {
                renderer->render(&renderPainter);
            } else {
                renderer->render(&renderPainter, elementId);
            }

            renderPainter.end();

            // Apply current color scheme if the svg asks for it
            if (applyColors) {
                QImage itmp = p.toImage();
                KIconEffect::colorize(itmp, theme->color(Theme::BackgroundColor), 1.0);
                p = p.fromImage(itmp);
            }

            theme->insertIntoCache(id, p);
            return p;
        }

        void createRenderer()
        {
            if (renderer) {
                return;
            }

            //kDebug() << kBacktrace();
            if (themed && path.isEmpty()) {
                path = Plasma::Theme::defaultTheme()->imagePath(themePath);
            }

            //kDebug() << "********************************";
            //kDebug() << "FAIL! **************************";
            //kDebug() << path << "**";

            QHash<QString, SharedSvgRenderer::Ptr>::const_iterator it = s_renderers.find(path);

            if (it != s_renderers.end()) {
                //kDebug() << "gots us an existing one!";
                renderer = it.value();
            } else {
                renderer = new SharedSvgRenderer(path);
                s_renderers[path] = renderer;
            }

            if (size == QSizeF()) {
                size = renderer->defaultSize();
            }
        }

        void eraseRenderer()
        {
            if (renderer && renderer.count() == 2) {
                // this and the cache reference it; and boy is this not thread safe ;)
                s_renderers.erase(s_renderers.find(path));
            }

            renderer = 0;
        }

        QRectF elementRect(const QString &elementId)
        {
            QRectF rect;

            if (themed && path.isEmpty()) {
                path = Plasma::Theme::defaultTheme()->imagePath(themePath);
            }

            bool found = Theme::defaultTheme()->findInRectsCache(path, cacheId(elementId), rect);

            if (found) {
                return rect;
            }

            return findAndCacheElementRect(elementId);
        }

        QRectF findAndCacheElementRect(const QString &elementId)
        {
            createRenderer();
            QRectF elementRect = renderer->elementExists(elementId) ?
                                 renderer->boundsOnElement(elementId) : QRectF();
            naturalSize = renderer->defaultSize();
            qreal dx = size.width() / naturalSize.width();
            qreal dy = size.height() / naturalSize.height();

            elementRect = QRectF(elementRect.x() * dx, elementRect.y() * dy,
                                 elementRect.width() * dx, elementRect.height() * dy);
            Theme::defaultTheme()->insertIntoRectsCache(path, cacheId(elementId), elementRect);

            return elementRect;
        }

        QMatrix matrixForElement(const QString &elementId)
        {
            createRenderer();
            return renderer->matrixForElement(elementId);
        }

        void checkApplyColorHint()
        {
            KConfigGroup cg(KGlobal::config(), "SvgHints");
            QString cgKey = themePath + "-hint-apply-color-scheme";
            if (cg.hasKey(cgKey)) {
                applyColors = cg.readEntry(cgKey, false);
            } else {
                createRenderer();
                applyColors = renderer->elementExists("hint-apply-color-scheme");
                cg.writeEntry(cgKey, applyColors);
            }
        }

        void themeChanged()
        {
            if (!themed) {
                return;
            }

            QString newPath = Theme::defaultTheme()->imagePath(themePath);

            if (path == newPath) {
                return;
            }

            path = newPath;
            //delete d->renderer; we're a KSharedPtr
            eraseRenderer();

            // check if new theme svg wants colorscheme applied
            bool wasApplyColors = applyColors;
            checkApplyColorHint();
            if (applyColors && !Theme::defaultTheme()->colorScheme()) {
                if (!wasApplyColors) {
                    QObject::connect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                     q, SLOT(colorsChanged()));
                }
            } else {
                QObject::disconnect(KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged()),
                                    q, SLOT(colorsChanged()));
            }

            //kDebug() << themePath << ">>>>>>>>>>>>>>>>>> theme changed";
            emit q->repaintNeeded();
        }

        void colorsChanged()
        {
            if (!applyColors) {
                return;
            }

            eraseRenderer();
            emit q->repaintNeeded();
        }

        Svg *q;
        static QHash<QString, SharedSvgRenderer::Ptr> s_renderers;
        SharedSvgRenderer::Ptr renderer;
        QString themePath;
        QString path;
        QSizeF size;
        QSizeF naturalSize;
        bool multipleImages;
        bool themed;
        bool applyColors;
};

QHash<QString, SharedSvgRenderer::Ptr> SvgPrivate::s_renderers;

Svg::Svg(QObject *parent)
    : QObject(parent),
      d(new SvgPrivate(this))
{
}

Svg::~Svg()
{
    delete d;
}

QPixmap Svg::pixmap(const QString &elementID)
{
    if (elementID.isNull() || d->multipleImages) {
        return d->findInCache(elementID, size());
    } else {
        return d->findInCache(elementID);
    }
}

void Svg::paint(QPainter *painter, const QPointF &point, const QString &elementID)
{
    QPixmap pix(elementID.isNull() ? d->findInCache(elementID, size()) :
                                     d->findInCache(elementID));

    if (pix.isNull()) {
        return;
    }

    painter->drawPixmap(QRectF(point, pix.size()), pix, QRectF(QPointF(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, const QString &elementID)
{
    paint(painter, QPointF(x, y), elementID);
}

void Svg::paint(QPainter *painter, const QRectF &rect, const QString &elementID)
{
    QPixmap pix(d->findInCache(elementID, rect.size()));
    painter->drawPixmap(rect, pix, QRectF(QPointF(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, int width, int height, const QString &elementID)
{
    QPixmap pix(d->findInCache(elementID, QSizeF(width, height)));
    painter->drawPixmap(x, y, pix, 0, 0, pix.size().width(), pix.size().height());
}

QSize Svg::size() const
{
    return d->size.toSize();
}

void Svg::resize(qreal width, qreal height)
{
    resize(QSize(width, height));
}

void Svg::resize(const QSizeF &size)
{
    d->size = size;
}

void Svg::resize()
{
    if (d->renderer) {
        d->size = d->renderer->defaultSize();
    } else {
        d->size = QSizeF();
    }
}

QSize Svg::elementSize(const QString &elementId) const
{
    return d->elementRect(elementId).size().toSize();
}

QRectF Svg::elementRect(const QString &elementId) const
{
    return d->elementRect(elementId);
}

bool Svg::hasElement(const QString &elementId) const
{
    if (d->path.isNull() && d->themePath.isNull()) {
        return false;
    }


    QRectF elementRect;
    bool found = Theme::defaultTheme()->findInRectsCache(d->path, d->cacheId(elementId), elementRect);

    if (found) {
        return elementRect.isValid();
    } else {
//        kDebug() << "** ** *** !!!!!!!! *** ** ** creating renderer due to hasElement miss" << d->path << elementId;
        d->findAndCacheElementRect(elementId);
        return d->renderer->elementExists(elementId);
    }
}

QString Svg::elementAtPoint(const QPoint &point) const
{
    return QString();
/*
FIXME: implement when Qt can support us!
    d->createRenderer();
    QSizeF naturalSize = d->renderer->defaultSize();
    qreal dx = d->size.width() / naturalSize.width();
    qreal dy = d->size.height() / naturalSize.height();
    //kDebug() << point << "is really"
    //         << QPoint(point.x() *dx, naturalSize.height() - point.y() * dy);

    return QString(); // d->renderer->elementAtPoint(QPoint(point.x() *dx, naturalSize.height() - point.y() * dy));
    */
}

bool Svg::isValid() const
{
    if (d->path.isNull() && d->themePath.isNull()) {
        return false;
    }

    d->createRenderer();
    return d->renderer->isValid();
}

void Svg::setContainsMultipleImages(bool multiple)
{
    d->multipleImages = multiple;
}

bool Svg::containsMultipleImages() const
{
    return d->multipleImages;
}

void Svg::setImagePath(const QString &svgFilePath)
{
    if (d->setImagePath(svgFilePath, this)) {
    }
        d->eraseRenderer();
        emit repaintNeeded();
}

QString Svg::imagePath() const
{
   return d->themed ? d->themePath : d->path;
}

} // Plasma namespace

#include "svg.moc"

