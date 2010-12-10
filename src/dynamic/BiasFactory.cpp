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

#include "BiasFactory.h"
#include "Bias.h"

#define DEBUG_PREFIX "BiasFactory"

#include "App.h"
#include "core/support/Debug.h"

#include <QList>
#include <QXmlStreamReader>

Dynamic::BiasFactory* Dynamic::BiasFactory::s_instance = 0;

QList<Dynamic::AbstractBiasFactory*> Dynamic::BiasFactory::s_biasFactories = QList<Dynamic::AbstractBiasFactory*>();

/*
QList< Dynamic::CustomBias* > Dynamic::CustomBias::s_biases = QList< Dynamic::CustomBias* >();
QMap< QString, Dynamic::CustomBias* > Dynamic::CustomBias::s_failedMap = QMap< QString, Dynamic::CustomBias* >();
QMap< QString, QDomElement > Dynamic::CustomBias::s_failedMapXml = QMap< QString, QDomElement >();
*/

Dynamic::BiasFactory*
Dynamic::BiasFactory::instance()
{
    if( !s_instance )
        s_instance = new BiasFactory( App::instance() );
    return s_instance;
}

Dynamic::BiasFactory::BiasFactory( QObject *parent )
    : QObject( parent )
{ }

Dynamic::AbstractBias*
Dynamic::BiasFactory::fromXml( QXmlStreamReader *reader, QObject *parent )
{
    QStringRef name = reader->name();

    foreach( Dynamic::AbstractBiasFactory* fac, instance()->factories() )
    {
        if( name == fac->name() )
            return fac->createBias( reader, parent );
    }

    if( name == Dynamic::AndBias::name() )
        return new Dynamic::AndBias( reader, parent );
    else if( name == Dynamic::OrBias::name() )
        return new Dynamic::OrBias( reader, parent );
    else if( name == Dynamic::TagMatchBias::name() )
        return new Dynamic::TagMatchBias( reader, parent );
    else
        return 0;
}

Dynamic::AbstractBias*
Dynamic::BiasFactory::fromName( const QString &name, QObject *parent )
{
    foreach( Dynamic::AbstractBiasFactory* fac, instance()->factories() )
    {
        if( name == fac->name() )
            return fac->createBias( parent );
    }

    if( name == Dynamic::AndBias::name() )
        return new Dynamic::AndBias( parent );
    else if( name == Dynamic::OrBias::name() )
        return new Dynamic::OrBias( parent );
    else if( name == Dynamic::TagMatchBias::name() )
        return new Dynamic::TagMatchBias( parent );
    else
        return 0;
}

void
Dynamic::BiasFactory::registerNewBiasFactory( Dynamic::AbstractBiasFactory* factory )
{
    debug() << "new factory of type:" << factory->name();
    if( !s_biasFactories.contains( factory ) )
        s_biasFactories.append( factory );

    /*
    foreach( const QString &name, s_failedMap.keys() )
    {
        if( name == entry->pluginName() ) // lazy loading!
        {
            debug() << "found entry loaded without proper custombiasentry. fixing now, with  old weight of" << s_failedMap[ name ]->weight() ;
            //  need to manually set the weight, as we set it on the old widget which is now being thrown away
            Dynamic::CustomBiasEntry* cbe = factory->newCustomBiasEntry( s_failedMapXml[ name ] );
            s_failedMap[ name ]->setCurrentEntry( cbe );
            s_failedMap.remove( name );
            s_failedMapXml.remove( name );
        }
    }
    */

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
    return s_biasFactories;
}

void
Dynamic::BiasFactory::emitChanged()
{
    emit changed();
}

#include "BiasFactory.moc"
