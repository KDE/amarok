/*****************************************************************************
 * Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                      *
 * Copyright (C) 2006 by Aaron J. Seigo <aseigo@kde.org>                     *
 * Copyright (C) 2009 by Seb Ruiz <ruiz@kde.org>                             *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef BREADCRUMBITEMBUTTON_P_H
#define BREADCRUMBITEMBUTTON_P_H

#include <QColor>

#include "widgets/ElidingButton.h"

class KUrl;
class QEvent;
class KUrlNavigator;

/**
 * @brief Base class for buttons of the URL navigator.
 *
 * Each button of the URL navigator contains an URL, which
 * is set as soon as the button has been clicked.
 */
class BreadcrumbItemButton : public Amarok::ElidingButton
{
    Q_OBJECT

public:
    BreadcrumbItemButton( QWidget* parent );
    BreadcrumbItemButton( const QIcon &icon, const QString &text, QWidget *parent );
    virtual ~BreadcrumbItemButton();

    void setActive( const bool active ) { setDisplayHintEnabled( ActiveHint, active ); }

protected:
    enum DisplayHint
    {
        ActiveHint = 1,
        HoverHint = 2
    };

    void setDisplayHintEnabled(DisplayHint hint, bool enable);
    bool isDisplayHintEnabled(DisplayHint hint) const;

    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);

    virtual void paintEvent(QPaintEvent* event);
    void drawHoverBackground(QPainter* painter);

    /** Returns the foreground color by respecting the current display hint. */
    QColor foregroundColor() const;

private:
    void init( QWidget *parent );
    int m_displayHint;
};

class BreadcrumbItemMenuButton : public BreadcrumbItemButton
{
    Q_OBJECT

public:
    explicit BreadcrumbItemMenuButton(QWidget* parent) : BreadcrumbItemButton(parent) { };
    virtual ~BreadcrumbItemMenuButton() { }

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent* event);
};

#endif
