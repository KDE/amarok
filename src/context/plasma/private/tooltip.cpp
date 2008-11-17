/*
 *   Copyright 2007 by Dan Meltzer <hydrogen@notyetimplemented.com>
 *   Copyright (C) 2008 by Alexis Ménard <darktears31@gmail.com>
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

#include "tooltip_p.h"
#include "windowpreview_p.h"

#include <QBitmap>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

#include <KDebug>
#include <KGlobal>

#include <plasma/plasma.h>
#include <plasma/theme.h>
#include <plasma/framesvg.h>

namespace Plasma {

class ToolTipPrivate
{
    public:
        ToolTipPrivate(QObject *s)
        : label(0),
          imageLabel(0),
          preview(0),
          windowToPreview(0),
          source(s),
          autohide(true)
    { }

    QLabel *label;
    QLabel *imageLabel;
    WindowPreview *preview;
    WId windowToPreview;
    FrameSvg *background;
    QPointer<QObject> source;
    bool autohide;
};

void ToolTip::showEvent(QShowEvent *e)
{
    QWidget::showEvent(e);
    d->preview->setInfo();
}

void ToolTip::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    if (d->source) {
        QMetaObject::invokeMethod(d->source, "toolTipHidden");
    }
}

void ToolTip::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos())) {
        hide();
    }
}

ToolTip::ToolTip(QObject *source)
    : QWidget(0),
      d(new ToolTipPrivate(source))
{
    if (source) {
        connect(source, SIGNAL(destroyed(QObject*)), this, SLOT(sourceDestroyed()));
    }

    setWindowFlags(Qt::ToolTip);
    QGridLayout *l = new QGridLayout;
    d->preview = new WindowPreview(this);
    d->label = new QLabel(this);
    d->label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    d->label->setWordWrap(true);
    d->imageLabel = new QLabel(this);
    d->imageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    d->background = new FrameSvg(this);
    connect(d->background, SIGNAL(repaintNeeded()), this, SLOT(update()));

    l->addWidget(d->preview, 0, 0, 1, 2);
    l->addWidget(d->imageLabel, 1, 0);
    l->addWidget(d->label, 1, 1);
    setLayout(l);
}

ToolTip::~ToolTip()
{
    delete d;
}

void ToolTip::setContent(const ToolTipManager::Content &data)
{
    //reset our size
    d->label->setText("<qt><b>" + data.mainText + "</b><br>" + data.subText + "</qt>");
    d->imageLabel->setPixmap(data.image);
    d->windowToPreview = data.windowToPreview;
    d->preview->setWindowId(d->windowToPreview);
    d->autohide = data.autohide;

    if (isVisible()) {
        resize(sizeHint());
    }
}

void ToolTip::prepareShowing(bool cueUpdate)
{
    if (cueUpdate && d->source) {
        QMetaObject::invokeMethod(d->source, "toolTipAboutToShow");
    }

    if (d->windowToPreview != 0) {
        // show/hide the preview area
        d->preview->show();
    } else {
        d->preview->hide();
    }

    layout()->activate();
    resize(sizeHint());
}

void ToolTip::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    d->background->resizeFrame(size());

    setMask(d->background->mask());
}

void ToolTip::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(e->rect());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);

    d->background->paintFrame(&painter);
}

void ToolTip::sourceDestroyed()
{
    d->source = 0;
}

bool ToolTip::autohide() const
{
    return d->autohide;
}

void ToolTip::updateTheme()
{
    d->background->setImagePath("widgets/tooltip");
    d->background->setEnabledBorders(FrameSvg::AllBorders);

    const int topHeight = d->background->marginSize(Plasma::TopMargin);
    const int leftWidth = d->background->marginSize(Plasma::LeftMargin);
    const int rightWidth = d->background->marginSize(Plasma::RightMargin);
    const int bottomHeight = d->background->marginSize(Plasma::BottomMargin);
    setContentsMargins(leftWidth, topHeight, rightWidth, bottomHeight);

    // Make the tooltip use Plasma's colorscheme
    QPalette plasmaPalette = QPalette();
    plasmaPalette.setColor(QPalette::Window,
                           Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
    plasmaPalette.setColor(QPalette::WindowText,
                           Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor));
    setAutoFillBackground(true);
    setPalette(plasmaPalette);
}

} // namespace Plasma

#include "tooltip_p.moc"
