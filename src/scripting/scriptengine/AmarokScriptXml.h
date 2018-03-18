/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef AMAROKSCRIPTXML_H
#define AMAROKSCRIPTXML_H

#include <QObject>

class QDomDocument;
class QScriptEngine;
class QXmlStreamReader;

namespace AmarokScript
{

class AmarokScriptXml : public QObject
{
    Q_OBJECT

public:
    explicit AmarokScriptXml( QScriptEngine *engine );
    ~AmarokScriptXml();

    Q_INVOKABLE void setReaderData( const QString &data );
    Q_INVOKABLE bool setDomObjectData( const QString &data );
    Q_INVOKABLE QString readFirstStreamElementWithName( const QString &name );
    Q_INVOKABLE QString textOfFirstDomElementWithName( const QString &name );

private:
    QXmlStreamReader *m_reader;
    QDomDocument *m_domDocument;
};

}

#endif // AMAROKSCRIPTXML_H
