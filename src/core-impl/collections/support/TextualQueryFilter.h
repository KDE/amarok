/****************************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2007-2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef TEXTUALQUERYFILTER_H
#define TEXTUALQUERYFILTER_H

#include "amarok_export.h"

#include "browsers/CollectionTreeItem.h"
#include "core/meta/forward_declarations.h"
#include "core/collections/QueryMaker.h"

#include <QDateTime>

namespace Collections
{
    /** Adds a conditions to the query maker specified in the filter.
        This is the engine behind the search field.
    */
    void addTextualFilter( Collections::QueryMaker *qm, const QString &filter );

    void addDateFilter( qint64 field, Collections::QueryMaker::NumberComparison compare,
                        bool negate, const QString &text, Collections::QueryMaker *qm );

    /** Returns a QDateTime from a text.
        e.g. converts "today" to the current date.

    */
    QDateTime semanticDateTimeParser( const QString &text, bool *absolute = nullptr );

}

#endif
