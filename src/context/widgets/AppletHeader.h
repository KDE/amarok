/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef CONTEXTAPPLETHEADER_H
#define CONTEXTAPPLETHEADER_H

#include "amarok_export.h"

#include <QGraphicsWidget>

class TextScrollingWidget;
class QGraphicsLinearLayout;

namespace Plasma {
    class IconWidget;
}

namespace Context
{

class AMAROK_EXPORT AppletHeader : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( QString titleText READ titleText WRITE setTitleText )
    Q_PROPERTY( qreal height READ height )

public:
    AppletHeader( QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0 );
    ~AppletHeader();

    qreal height() const;

    QString titleText() const;
    void setTitleText( const QString &text );

    void addIcon( Plasma::IconWidget *icon, Qt::Alignment align );

    TextScrollingWidget *textScrollingWidget();

private:
    void clearDummyItems();

    qreal m_height;
    QGraphicsLinearLayout *m_mainLayout;
    QGraphicsLinearLayout *m_leftLayout;
    QGraphicsLinearLayout *m_rightLayout;
    QList<QGraphicsLayoutItem*> m_dummyItems;
    TextScrollingWidget *m_titleWidget;
    Q_DISABLE_COPY( AppletHeader )
};

} // namespace Context

#endif // CONTEXTAPPLETHEADER_H
