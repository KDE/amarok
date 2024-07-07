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

#include "AmarokScriptXml.h"

#include "core/support/Debug.h"

#include <QDomDocument>
#include <QJSEngine>
#include <QXmlStreamReader>

using namespace AmarokScript;

AmarokScriptXml::AmarokScriptXml( QJSEngine *engine )
    : QObject( engine )
    , m_reader( nullptr )
    , m_domDocument( new QDomDocument )
{
    QJSValue scriptObject = engine->newQObject( this );
    engine->globalObject().property( QStringLiteral("Amarok") ).setProperty( QStringLiteral("Xml"), scriptObject );
}

AmarokScriptXml::~AmarokScriptXml()
{
    delete m_domDocument;

    if( m_reader )
        delete m_reader;
}

void AmarokScriptXml::setReaderData(const QString& data)
{
    if( m_reader )
        delete m_reader;

    m_reader = new QXmlStreamReader( data );
}

bool AmarokScriptXml::setDomObjectData(const QString& data)
{
    return static_cast<bool>( m_domDocument->setContent( data ) );
}

QString AmarokScriptXml::readFirstStreamElementWithName(const QString& name)
{
    if( !m_reader )
        return QString();

    while( m_reader->readNextStartElement() )
    {
        if( m_reader->name() == name )
            return m_reader->readElementText();
    }

    return QString();
}

QString AmarokScriptXml::textOfFirstDomElementWithName(const QString& name)
{
    auto elements = m_domDocument->elementsByTagName( name );

    if( !elements.isEmpty() )
    {
        auto element = elements.at( 0 );
        return element.toElement().text();
    }

    return QString();
}
