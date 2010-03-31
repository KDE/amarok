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

#ifndef APG_TREECONTROLLER
#define APG_TREECONTROLLER

#include <QObject>

class QLayout;
class QTreeView;
class QWidget;

namespace APG {
    class TreeModel;

    class TreeController : public QObject {
        Q_OBJECT

        public:
            TreeController( TreeModel*, QTreeView*, QWidget* parent = 0);
            ~TreeController();

        public slots:
            void addGroup() const;
            void addConstraint( const QString& ) const;
            void removeNode() const;

        private:
            TreeModel* m_model;
            QTreeView* m_view;
    };
}
#endif
