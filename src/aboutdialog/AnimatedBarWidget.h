/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2012 Lachlan Dufton <dufton@gmail.com>                                 *
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

#ifndef AMAROK_ANIMATEDBARWIDGET_H
#define AMAROK_ANIMATEDBARWIDGET_H

#include "AnimatedWidget.h"

#include <QAbstractButton>
#include <QLabel>

class AnimatedBarWidget : public QAbstractButton
{
    Q_OBJECT
public:
    AnimatedBarWidget( const QIcon &icon, const QString &text, const QString &animatedIconName = QStringLiteral("process-working"), QWidget *parent = nullptr );
    ~AnimatedBarWidget() override;

    bool isAnimating() const { return m_animating; }

    QSize sizeHint() const override;
    int heightForWidth(int w) const override;

public Q_SLOTS:
    void animate();
    void stop();
    void fold();

protected:
    void setHoverHintEnabled( bool enable);
    bool isHoverHintEnabled() const;

    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

    void paintEvent(QPaintEvent* event) override;
    void drawHoverBackground(QPainter* painter);

    //! Returns the foreground color by respecting the current display hint.
    QColor foregroundColor() const;

private:
    void init();
    bool m_hoverHint;
    AnimatedWidget *m_animatedWidget;
    bool m_animating;
    QIcon m_icon;
};



#endif //AMAROK_ANIMATEDBARWIDGET_H
