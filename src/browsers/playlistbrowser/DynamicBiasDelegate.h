/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_DYNAMICBIASDELEGATE
#define AMAROK_DYNAMICBIASDELEGATE

#include <QStyledItemDelegate>

namespace PlaylistBrowserNS
{

/** A special delegate for the dynamic playlists.
    It will paint an additional operator hint in front of a bias.
    e.g. a small progress bar or a "If" text.
*/
class DynamicBiasDelegate : public QStyledItemDelegate
{
    public:
        explicit DynamicBiasDelegate( QWidget* parent = nullptr );
        ~DynamicBiasDelegate() override;

        void paint( QPainter* painter,
                    const QStyleOptionViewItem& option,
                    const QModelIndex& index ) const override;

    private:
      QFont m_normalFont;
      QFont m_smallFont;

      QFontMetrics *m_smallFm;

};

}

#endif
