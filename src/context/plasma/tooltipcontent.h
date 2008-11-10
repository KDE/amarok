/*
 * Copyright 2008 by Aaron Seigo <aseigo@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#include <QtCore/QString>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>

#include <plasma/plasma_export.h>

/**
 * This provides the content for a tooltip.
 *
 * Normally you will want to set at least the @p mainText and
 * @p subText.
 */

namespace Plasma
{

class ToolTipContentPrivate;

class PLASMA_EXPORT ToolTipContent
{
public:
    /** Creates an empty Content */
    ToolTipContent();

    ~ToolTipContent();

    /** Copy constructor */
    ToolTipContent(const ToolTipContent &other);

    /** Constructor that sets the common fields */
    ToolTipContent(const QString &mainText,
                   const QString &subText,
                   const QPixmap &image = QPixmap());

    /** Constructor that sets the common fields */
    ToolTipContent(const QString &mainText,
                   const QString &subText,
                   const QIcon &icon);

    ToolTipContent &operator=(const ToolTipContent &other);

    /** @return true if all the fields are empty */
    bool isEmpty() const;

    /** Sets the main text which containts important information, e.g. the title */
    void setMainText(const QString &text);

    /** Important information, e.g. the title */
    QString mainText() const;

    /** Sets text which elaborates on the @p mainText */
    void setSubText(const QString &text) ;

    /** Elaborates on the @p mainText */
    QString subText() const;

    /** Sets the icon to show **/
    void setImage(const QPixmap &image);

    /** Sets the icon to show **/
    void setImage(const QIcon &icon);

    /** An icon to display */
    QPixmap image() const;

    /** Sets the ID of the window to show a preview for */
    void setWindowToPreview(WId id);

    /** Id of a window if you want to show a preview */
    WId windowToPreview() const;

    /** Sets whether or not to autohide the tooltip, defaults to true */
    void setAutohide(bool autohide);

    /** Whether or not to autohide the tooltip, defaults to true */
    bool autohide() const;

private:
    ToolTipContentPrivate * const d;
};

} // namespace Plasma


