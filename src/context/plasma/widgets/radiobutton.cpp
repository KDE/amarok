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

#include "radiobutton.h"

#include <QRadioButton>
#include <QPainter>
#include <QDir>

#include <KMimeType>

#include "theme.h"
#include "svg.h"

namespace Plasma
{

class RadioButtonPrivate
{
public:
    RadioButtonPrivate()
        : svg(0)
    {
    }

    ~RadioButtonPrivate()
    {
        delete svg;
    }

    void setPixmap(RadioButton *q)
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

        static_cast<QRadioButton*>(q->widget())->setIcon(QIcon(pm));
    }

    QString imagePath;
    QString absImagePath;
    Svg *svg;
};

RadioButton::RadioButton(QGraphicsWidget *parent)
    : QGraphicsProxyWidget(parent),
      d(new RadioButtonPrivate)
{
    QRadioButton *native = new QRadioButton;
    connect(native, SIGNAL(toggled(bool)), this, SIGNAL(toggled(bool)));
    setWidget(native);
    native->setAttribute(Qt::WA_NoSystemBackground);
}

RadioButton::~RadioButton()
{
    delete d;
}

void RadioButton::setText(const QString &text)
{
    static_cast<QRadioButton*>(widget())->setText(text);
}

QString RadioButton::text() const
{
    return static_cast<QRadioButton*>(widget())->text();
}

void RadioButton::setImage(const QString &path)
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

QString RadioButton::image() const
{
    return d->imagePath;
}

void RadioButton::setStyleSheet(const QString &stylesheet)
{
    widget()->setStyleSheet(stylesheet);
}

QString RadioButton::styleSheet()
{
    return widget()->styleSheet();
}

QRadioButton *RadioButton::nativeWidget() const
{
    return static_cast<QRadioButton*>(widget());
}

void RadioButton::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    d->setPixmap(this);
    QGraphicsProxyWidget::resizeEvent(event);
}

void RadioButton::setChecked(bool checked)
{
    static_cast<QRadioButton*>(widget())->setChecked(checked);
}

bool RadioButton::isChecked() const
{
    return static_cast<QRadioButton*>(widget())->isChecked();
}

} // namespace Plasma

#include <radiobutton.moc>

