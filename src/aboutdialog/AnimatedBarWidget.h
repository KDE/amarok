/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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
    AnimatedBarWidget( const QIcon &icon, const QString &text, const QString &animatedIconName = "process-working", QWidget *parent = 0 );
    ~AnimatedBarWidget();

    bool isAnimating() const { return m_animating; }

    virtual QSize sizeHint() const;

public slots:
    void animate();
    void stop();
    void fold();

protected:
    void setHoverHintEnabled( bool enable);
    bool isHoverHintEnabled() const;

    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

    virtual void paintEvent(QPaintEvent* event);
    void drawHoverBackground(QPainter* painter);

    //! Returns the foreground color by respecting the current display hint.
    QColor foregroundColor() const;

private:
    void init();
    bool m_hoverHint;
    AnimatedWidget *m_animatedWidget;
    QString *m_text;
    bool m_animating;
    QIcon m_icon;
};



#endif //AMAROK_ANIMATEDBARWIDGET_H
