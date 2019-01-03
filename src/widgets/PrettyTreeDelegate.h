/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMAROK_PRETTY_TREE_DELEGATE_H
#define AMAROK_PRETTY_TREE_DELEGATE_H

#include <QFont>
#include <QRect>
#include <QStyledItemDelegate>

namespace Amarok {
    class PrettyTreeView;
}
class QFontMetrics;

/** A delegate used for the browser.
    This delegate has the following specialities:
     It will handle the hasCoverRole and will draw a bigger item for the cover.
     It will handle the byLineRole and will draw an extra big item with a second
      line of text
     Also this big item will have an expander arrow if needed and capacities and
     actions.
*/
class PrettyTreeDelegate : public QStyledItemDelegate
{
    Q_OBJECT

    public:
        explicit PrettyTreeDelegate( Amarok::PrettyTreeView *view );
        ~PrettyTreeDelegate();

        void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const override;
        QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const override;

        /** Returns the rectangle where the action icon with the specific nr is located. */
        QRect decoratorRect( const QRect &itemRect, int nr ) const;

    private:
        /** Verify and if needed update the buffered fonts and font metrics. */
        void updateFonts( const QStyleOptionViewItem &option ) const;

        Amarok::PrettyTreeView *m_view;

        mutable QFont m_originalFont;
        mutable QFont m_bigFont;
        mutable QFont m_smallFont;

        mutable QFontMetrics *m_normalFm;
        mutable QFontMetrics *m_bigFm;
        mutable QFontMetrics *m_smallFm;
};

#endif
