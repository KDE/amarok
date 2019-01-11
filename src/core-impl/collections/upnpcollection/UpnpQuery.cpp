/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#define DEBUG_PREFIX "UpnpQuery"

#include "UpnpQuery.h"

#include "core/support/Debug.h"

UpnpQuery::UpnpQuery()
{
    reset();
}

void UpnpQuery::reset()
{
    m_stack.clear();
    m_expressions.clear();
    m_andStack.clear();
    m_andStack.push( true );
    m_hasMatchFilter = false;
}

QStringList UpnpQuery::queries()
{
    return m_expressions;
}

void UpnpQuery::setType(const QString& type )
{
    m_expressions.append( type );
}

void UpnpQuery::beginAnd()
{
    for( int i = 0, total = m_expressions.length(); i < total ; ++i ) {
        m_expressions[i] += " and ";
    }
}

void UpnpQuery::beginOr()
{
    if( m_andStack.top() ) {
        m_stack.push( m_expressions );
        m_expressions.clear();
    }
    m_andStack.push( false );
}

void UpnpQuery::endAndOr()
{
    m_andStack.pop();

    if( m_andStack.empty() )
        return;

    if( m_andStack.top() ) {
        ExpressionList top = m_stack.pop();
        ExpressionList copy = ExpressionList( m_expressions );
        m_expressions.clear();

        if( copy.isEmpty() ) {
            m_expressions = top;
        }
        else if( top.isEmpty() ) {
            m_expressions = copy;
        }
        else {
            foreach( const QString &stackElem, top )
                foreach( const QString &copyElem, copy )
                    m_expressions.append( stackElem + " and " + copyElem );
        }
    }
}

void UpnpQuery::addFilter(const QString& filter )
{
    m_hasMatchFilter = true;
    m_expressions.append( filter );
}

void UpnpQuery::addMatch(const QString& match )
{
    m_hasMatchFilter = true;
    for( int i = 0; i < m_expressions.length() ; ++i ) {
        m_expressions[i] += " and ";
        m_expressions[i] += match;
    }
}


