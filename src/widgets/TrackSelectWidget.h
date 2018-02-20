/****************************************************************************************
 * Copyright (c) 2008-2010 Soren Harward <stharward@gmail.com>                          *
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

#ifndef AMAROK_TRACK_SELECT_WIDGET_H
#define AMAROK_TRACK_SELECT_WIDGET_H

#include "core/meta/forward_declarations.h"
#include "widgets/BoxWidget.h"

#include <QString>

class CollectionTreeItem;
class CollectionTreeItemModel;
class CollectionTreeView;

class KSqueezedTextLabel;

class TrackSelectWidget: public BoxWidget
{
    Q_OBJECT

    public:
        TrackSelectWidget( QWidget* parent );
        ~TrackSelectWidget();

        void setData( const Meta::DataPtr& );

    Q_SIGNALS:
        void selectionChanged( const Meta::DataPtr& );

    private Q_SLOTS:
        void recvNewSelection( CollectionTreeItem* );

    private:
        CollectionTreeView* m_view;
        CollectionTreeItemModel* m_model;
        KSqueezedTextLabel* m_label;

        const QString dataToLabel( const Meta::DataPtr& ) const;
};

#endif // AMAROK_TRACK_SELECT_WIDGET_H
