/****************************************************************************************
 * Copyright (c) 2007-2008 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_XMLQUERYREADER_H
#define AMAROK_XMLQUERYREADER_H

#include "collection/QueryMaker.h"

#include <QList>
#include <QXmlStreamReader>

class QueryMaker;

class XmlQueryReader : public QXmlStreamReader
{
public:

    enum ReturnValueEnum { IgnoreReturnValues = 0
                           , ParseReturnValues
                         };

    static QueryMaker* getQueryMaker( const QString &xmlData, ReturnValueEnum flag );

    XmlQueryReader( QueryMaker *qm, ReturnValueEnum flag );
    virtual ~XmlQueryReader();

    bool read( const QString &xmlData );

    struct Filter
    {
        Filter() : exclude(false), field(0), compare(-1) {}

        bool     exclude;
        qint64   field;
        QString  value;        
        int      compare; /* -1 => not a numerical comparison */
    };

    const QList<Filter>& getFilters() const;


private:
    void readQuery();
    void readFilters();
    void readReturnValues();
    void ignoreElements();
    void readAndOr();

    qint64 fieldVal( QStringRef field );
    int compareVal( QStringRef compare );

    struct Private;
    Private * const d;
};

#endif
