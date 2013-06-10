/****************************************************************************************
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>
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

#ifndef AMAROK_COLLECTION_NEPOMUKINQUIRER_H
#define AMAROK_COLLECTION_NEPOMUKINQUIRER_H

#include <memory>

#include <ThreadWeaver/Job>

#include <QString>

namespace Collections {

class NepomukParser;

class NepomukInquirerPrivate;

/**
 * A ThreadWeaver Job that runs the given SPARQL query on a Nepomuk's
 * database and passes the results to a given parser.
 */
class NepomukInquirer: public ThreadWeaver::Job
{
    Q_OBJECT

public:
    NepomukInquirer( QString query, std::auto_ptr<NepomukParser> parser );
    ~NepomukInquirer();

protected:
    void run();

private:
    QString m_query;
    NepomukParser *m_parser;
};

}

#endif // AMAROK_COLLECTION_NEPOMUKINQUIRER_H
