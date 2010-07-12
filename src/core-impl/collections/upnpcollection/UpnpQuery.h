/*
    Copyright (C) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef UPNPQUERY_H
#define UPNPQUERY_H

#include <QList>
#include <QStringList>
#include <QStack>

class UpnpQuery
{

    public:
        UpnpQuery();
        void reset();
        void setType( const QString & );
        void beginAnd();
        void beginOr();
        void endAndOr();
        void addFilter( const QString & );
        void addMatch( const QString & );
        QStringList queries();

    private:
        // poor man's tree
        typedef QStack<QStringList> ExpressionListStack;
        typedef QStringList ExpressionList;

        ExpressionListStack m_stack;
        ExpressionList m_expressions;
        QStack<bool> m_andStack;
};

#endif // UPNPQUERY_H
