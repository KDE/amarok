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

#include "label.h"

#include <QLabel>
#include <QPainter>
#include <QDir>

#include <KMimeType>

#include "theme.h"
#include "svg.h"

namespace Plasma
{

class LabelPrivate
{
public:
    LabelPrivate(Label *label)
        : q(label),
          svg(0)
    {
    }

    ~LabelPrivate()
    {
        delete svg;
    }

    void setPixmap(Label *q)
    {
        if (imagePath.isEmpty()) {
            return;
        }

        KMimeType::Ptr mime = KMimeType::findByPath(absImagePath);
        QPixmap pm(q->size().toSize());

        if (mime->is("image/svg+xml") || mime->is("application/x-gzip")) {
            svg = new Svg();
            svg->setImagePath(imagePath);
            QPainter p(&pm);
            svg->paint(&p, pm.rect());
        } else {
            pm = QPixmap(absImagePath);
        }

        static_cast<QLabel*>(q->widget())->setPixmap(pm);
    }

    void setPalette()
    {
        QLabel *native = q->nativeWidget();
        QColor color = Theme::defaultTheme()->color(Theme::TextColor);
        QPalette p = native->palette();
        p.setColor(QPalette::Normal, QPalette::WindowText, color);
        p.setColor(QPalette::Inactive, QPalette::WindowText, color);
        native->setPalette(p);
    }

    Label *q;
    QString imagePath;
    QString absImagePath;
    Svg *svg;
};

Label::Label(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new LabelPrivate(this))
{
    QLabel *native = new QLabel;
    connect(native, SIGNAL(linkActivated(QString)), this, SIGNAL(linkActivated(QString)));

    connect(Theme::defaultTheme(), SIGNAL(themeChanged()), this, SLOT(setPalette()));
    native->setAttribute(Qt::WA_NoSystemBackground);
    native->setWordWrap(true);
    setWidget(native);
    d->setPalette();
}

Label::~Label()
{
    delete d;
}

void Label::setText(const QString &text)
{
    static_cast<QLabel*>(widget())->setText(text);
}

QString Label::text() const
{
    return static_cast<QLabel*>(widget())->text();
}

void Label::setImage(const QString &path)
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

QString Label::image() const
{
    return d->imagePath;
}

void Label::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString Label::styleSheet()
{
    return widget()->styleSheet();
}

QLabel *Label::nativeWidget() const
{
    return static_cast<QLabel*>(widget());
}

void Label::dataUpdated(const QString &sourceName, const Plasma::DataEngine::Data &data)
{
    Q_UNUSED(sourceName);

    QStringList texts;
    foreach (const QVariant &v, data) {
        if (v.canConvert(QVariant::String)) {
            texts << v.toString();
        }
    }

    setText(texts.join(" "));
}

void Label::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->setPixmap(this);
    QGraphicsProxyWidget::resizeEvent(event);
}

} // namespace Plasma

#include <label.moc>

