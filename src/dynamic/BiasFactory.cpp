/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "BiasFactory"

#include "BiasFactory.h"

#include "App.h"
#include "biases/AlbumPlayBias.h"
#include "biases/IfElseBias.h"
#include "biases/PartBias.h"
#include "biases/TagMatchBias.h"
#include "biases/SearchQueryBias.h"
#include "biases/QuizPlayBias.h"
#include "biases/EchoNestBias.h"
#include "core/support/Debug.h"
#include "core/collections/QueryMaker.h"
#include "dynamic/Bias.h"
#include "scripting/scriptengine/exporters/ScriptableBiasExporter.h"

#include <QFormLayout>
#include <QLabel>
#include <QList>
#include <QXmlStreamReader>

Dynamic::BiasPtr
Dynamic::AbstractBiasFactory::createFromXml( QXmlStreamReader *reader )
{
    Dynamic::BiasPtr bias( createBias() );
    bias->fromXml( reader );
    return bias;
}

class RandomBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const override
    { return i18nc("Name of the random bias", "Random"); }

    QString name() const override
    { return Dynamic::RandomBias::sName(); }

    QString i18nDescription() const override
    { return i18nc("Description of the random bias",
                   "The random bias adds random tracks from the\n"
                   "whole collection without any bias."); }

    Dynamic::BiasPtr createBias() override
    { return Dynamic::BiasPtr( new Dynamic::RandomBias() ); }
};


class AndBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const override
    { return i18nc("Name of the \"And\" bias", "And"); }

    QString name() const override
    { return Dynamic::AndBias::sName(); }

    QString i18nDescription() const override
    { return i18nc("Description of the \"And\" bias",
                   "The \"And\" bias adds tracks that match all\n"
                   "of the sub biases."); }

    Dynamic::BiasPtr createBias() override
    { return Dynamic::BiasPtr( new Dynamic::AndBias() ); }
};


class OrBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const override
    { return i18nc("Name of the \"Or\" bias", "Or"); }

    QString name() const override
    { return Dynamic::OrBias::sName(); }

    QString i18nDescription() const override
    { return i18nc("Description of the \"Or\" bias",
                   "The \"Or\" bias adds tracks that match at\n"
                   "least one of the sub biases."); }

    Dynamic::BiasPtr createBias() override
    { return Dynamic::BiasPtr( new Dynamic::OrBias() ); }
};

Dynamic::BiasFactory* Dynamic::BiasFactory::s_instance = nullptr;

QList<Dynamic::AbstractBiasFactory*> Dynamic::BiasFactory::s_biasFactories = QList<Dynamic::AbstractBiasFactory*>();

Dynamic::BiasFactory*
Dynamic::BiasFactory::instance()
{
    if( !s_instance )
    {
        // --- build in biases
        s_biasFactories.append( new Dynamic::SearchQueryBiasFactory() );
        s_biasFactories.append( new RandomBiasFactory() );
        s_biasFactories.append( new AndBiasFactory() );
        s_biasFactories.append( new OrBiasFactory() );
        s_biasFactories.append( new Dynamic::PartBiasFactory() );
        s_biasFactories.append( new Dynamic::IfElseBiasFactory() );
        s_biasFactories.append( new Dynamic::TagMatchBiasFactory() );
        s_biasFactories.append( new Dynamic::AlbumPlayBiasFactory() );
        s_biasFactories.append( new Dynamic::QuizPlayBiasFactory() );
        s_biasFactories.append( new Dynamic::EchoNestBiasFactory() );

        s_instance = new BiasFactory( pApp );
    }
    return s_instance;
}



// --------------- ReplacementBias -------------


Dynamic::ReplacementBias::ReplacementBias( const QString &n )
    : m_name( n )
{
    connect( BiasFactory::instance(), &Dynamic::BiasFactory::changed, this, &ReplacementBias::factoryChanged );
}

Dynamic::ReplacementBias::ReplacementBias( const QString &n, QXmlStreamReader *reader )
    : m_name( n )
{
    // -- read the original bias data as one block
    quint64 start = reader->characterOffset();
    reader->skipCurrentElement();
    quint64 end = reader->characterOffset();

    QIODevice *device = reader->device();
    if( device->isSequential() )
    {
        warning() << "Cannot read xml for bias"<<n<<"from sequential device.";
        return;
    }
    device->seek( start );
    m_html = device->read( end - start );

    debug() << "replacement bias for"<<n<<"is"<<m_html;

    connect( BiasFactory::instance(), &Dynamic::BiasFactory::changed, this, &ReplacementBias::factoryChanged );
}

void
Dynamic::ReplacementBias::toXml( QXmlStreamWriter *writer ) const
{
    Q_UNUSED( writer );
    writer->writeComment(QStringLiteral("Replacement")); // we need to force the closing of the bias start tag
    writer->device()->write( m_html.left( m_html.size() - m_name.length() - 3 ) );
}

QString
Dynamic::ReplacementBias::sName()
{
    return QStringLiteral( "replacementBias" );
}

QString
Dynamic::ReplacementBias::name() const
{
    return m_name;
}

QString
Dynamic::ReplacementBias::toString() const
{
    return i18n( "Replacement for bias %1", m_name );
}

QWidget*
Dynamic::ReplacementBias::widget( QWidget* parent )
{
    QLabel *label = new QLabel( i18n( "Replacement for bias %1", m_name ), parent );

    return label;
}

void
Dynamic::ReplacementBias::factoryChanged()
{
    DEBUG_BLOCK;

    // -- search if there is a new factory with my name
    for( AbstractBiasFactory* factory : BiasFactory::instance()->factories() )
    {
        if( factory->name() == m_name )
        {
            debug() << "Found new factory for" << m_name;

            // -- replace myself with the new bias
            QXmlStreamReader reader( m_html );

            Dynamic::BiasPtr newBias( factory->createFromXml( &reader ) );
            replace( newBias );
            return;
        }
    }
}


// ------------- BiasFactory --------------

Dynamic::BiasFactory::BiasFactory( QObject *parent )
    : QObject( parent )
{ }

Dynamic::BiasFactory::~BiasFactory()
{
    qDeleteAll(s_biasFactories);
}

Dynamic::BiasPtr
Dynamic::BiasFactory::fromXml( QXmlStreamReader *reader )
{
    QStringRef name = reader->name();

    instance(); // ensure that we have an instance with the default factories
    for( Dynamic::AbstractBiasFactory* fac : s_biasFactories )
    {
        if( name == fac->name() )
            return fac->createFromXml( reader );
    }
    return Dynamic::BiasPtr( new ReplacementBias( name.toString(), reader ) );
}

Dynamic::BiasPtr
Dynamic::BiasFactory::fromName( const QString &name )
{
    instance(); // ensure that we have an instance with the default factories
    for( Dynamic::AbstractBiasFactory* fac : s_biasFactories )
    {
        if( name == fac->name() )
            return fac->createBias();
    }
    return Dynamic::BiasPtr( new ReplacementBias( name ) );
}

void
Dynamic::BiasFactory::registerNewBiasFactory( Dynamic::AbstractBiasFactory* factory )
{
    instance(); // ensure that we have an instance with the default factories
    debug() << "new factory of type:" << factory->name();
    if( !s_biasFactories.contains( factory ) )
        s_biasFactories.append( factory );

    instance()->emitChanged();
}

void
Dynamic::BiasFactory::removeBiasFactory( Dynamic::AbstractBiasFactory* factory )
{
    if( s_biasFactories.contains( factory ) )
        s_biasFactories.removeAll( factory );

    instance()->emitChanged();
}

QList<Dynamic::AbstractBiasFactory*>
Dynamic::BiasFactory::factories()
{
    instance(); // ensure that we have an instance with the default factories
    return s_biasFactories;
}

void
Dynamic::BiasFactory::emitChanged()
{
    Q_EMIT changed();
}

