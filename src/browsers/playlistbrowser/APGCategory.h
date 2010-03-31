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

#ifndef APGCATEGORY_H
#define APGCATEGORY_H

#include "browsers/BrowserCategory.h"

#include <QModelIndex>

namespace PlaylistBrowserNS {

    /* Playlist Browser toolbox item for the Automatic Playlist Generator */
    class APGCategory : public BrowserCategory {
        Q_OBJECT

        public:
            APGCategory( QWidget* parent );
            ~APGCategory();

        signals:
            void validIndexSelected( bool );

        private slots:
            void activeChanged( const QModelIndex& );
            void setQualityFactor( int );
            void runGenerator();

        private:
            int m_qualityFactor;
    };

}

#endif
