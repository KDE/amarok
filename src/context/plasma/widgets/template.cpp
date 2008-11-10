/*
 *   Copyright 2008 Aaron Seigo <aseigo@kde.org>
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

#include "<name>.h"

#include <<Native>>
#include <QPainter>

#include <KMimeType>

#include "theme.h"
#include "svg.h"

namespace Plasma
{

class <Name>Private
{
public:
    <Name>Private()
        : svg(0)
    {
    }

    ~<Name>Private()
    {
        delete svg;
    }

    void setPixmap(<Name> *q)
    {
        if (imagePath.isEmpty()) {
            return;
        }

        KMimeType::Ptr mime = KMimeType::findByPath(absImagePath);
        QPixmap pm(q->size().toSize());

        if (mime->is("image/svg+xml")) {
            svg = new Svg();
            QPainter p(&pm);
            svg->paint(&p, pm.rect());
        } else {
            pm = QPixmap(absImagePath);
        }

        //TODO: load image into widget
        //static_cast<<Native>*>(q->widget())->setPixmappm(pm);
        //static_cast<<Native>*>(q->widget())->setIcon(QIcon(pm));
    }

    QString imagePath;
    QString absImagePath;
    Svg *svg;
};

<Name>::<Name>(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new <Name>Private)
{
    <Native>* native = new <Native>;
    //TODO: forward signals
    //connect(native, SIGNAL(()), this, SIGNAL(()));
    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);
}

<Name>::~<Name>()
{
    delete d;
}

void <Name>::setText(const QString &text)
{
    static_cast<<Native>*>(widget())->setText(text);
}

QString <Name>::text() const
{
    return static_cast<<Native>*>(widget())->text();
}

void <Name>::setImage(const QString &path)
{
    if (d->imagePath == path) {
        return;
    }

    delete d->svg;
    d->svg = 0;
    d->imagePath = path;

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

    d->setPixmap(this);
}

QString <Name>::image() const
{
    return d->imagePath;
}

void <Name>::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString <Name>::styleSheet()
{
    return widget()->styleSheet();
}

<Native>* <Name>::nativeWidget() const
{
    return static_cast<<Native>*>(widget());
}

void <Name>::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->setPixmap(this);
    QGraphicsProxyWidget::resizeEvent(event);
}

} // namespace Plasma

#include <<name>.moc>

