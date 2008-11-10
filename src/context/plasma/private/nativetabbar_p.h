/*
    Copyright 2007 Robert Knight <robertknight@gmail.com>
    Copyright 2008 Marco Martin <notmart@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef NATIVETABBAR_H
#define NATIVETABBAR_H

#include <QTabBar>

namespace Plasma
{

class NativeTabBarPrivate;

class NativeTabBar : public QTabBar
{
    Q_OBJECT

public:
    NativeTabBar(QWidget *parent = 0);
    ~NativeTabBar();

    QRect tabRect(int index) const;
    QSize tabSizeHint(int index) const;
    QSize sizeHint() const;

protected:
    int lastIndex() const;

    // reimplemented from QTabBar
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
    void tabInserted(int index);
    void tabRemoved(int index);
    void tabLayoutChange();

    bool isHorizontal() const;
    bool isVertical() const;

protected slots:
    void animationFinished();
    void startAnimation();
    void onValueChanged(qreal val);

Q_SIGNALS:
    void sizeHintChanged();
    void shapeChanged(QTabBar::Shape shape);

private:
    QSize tabSize(int index) const;

    NativeTabBarPrivate * const d;

    friend class NativeTabBarPrivate;

    Q_PRIVATE_SLOT(d, void syncBorders())
};

}

#endif // TABBAR_H
