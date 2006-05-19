/*
  Copyright (c) 2006 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROK_EXPRESSION_H
#define AMAROK_EXPRESSION_H

#include <qstring.h>
#include <qvaluevector.h>

struct expression_element
{
    QString field;
    QString text;
    bool negate: 1;
    enum { Contains, Less, More } match: 2;
    expression_element(): negate( false ), match( Contains ) { }
};
typedef QValueVector<expression_element> or_list;

typedef QValueVector<or_list> ParsedExpression;

class ExpressionParser
{
    public:
        ExpressionParser( const QString &expression );
        ParsedExpression parse();
        static ParsedExpression parse( const QString &expression );

        static bool isAdvancedExpression( const QString &expression );

    private:
        void parseChar(   const QChar &c );
        void handleSpace( const QChar &c );
        void handleMinus( const QChar &c );
        void handleColon( const QChar &c );
        void handleMod(   const QChar &c );
        void handleQuote( const QChar &c );
        void handleChar(  const QChar &c );
        void finishedToken();
        void finishedElement();
        void finishedOrGroup();

        const QString &m_expression;
        enum State { ExpectMinus, ExpectField, ExpectMod, ExpectText };
        int m_state;
        bool m_haveGroup;
        bool m_inQuote;
        bool m_inOrGroup;
        QString m_string;
        expression_element m_element;
        or_list m_or;
        ParsedExpression m_parsed;
};


#endif
