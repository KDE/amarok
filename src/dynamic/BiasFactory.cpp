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
#include "biases/AlbumPlayBias.h"
#include "biases/PartBias.h"
#include "biases/TagMatchBias.h"
#include "biases/EchoNestBias.h"

#define DEBUG_PREFIX "BiasFactory"

#include "App.h"
#include "core/support/Debug.h"

#include <QList>
#include <QXmlStreamReader>


class RandomBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const
    { return i18nc("Name of the random bias", "Random"); }

    QString name() const
    { return Dynamic::RandomBias::sName(); }

    QString i18nDescription() const
    { return i18nc("Description of the random bias",
                   "The random bias adds random tracks from the whole collection without any bias."); }

    Dynamic::BiasPtr createBias()
    { return Dynamic::BiasPtr( new Dynamic::RandomBias() ); }

    Dynamic::BiasPtr createBias( QXmlStreamReader *reader )
    { return Dynamic::BiasPtr( new Dynamic::RandomBias( reader ) ); }
};


class UniqueBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const
    { return i18nc("Name of the unique bias", "Unique"); }

    QString name() const
    { return Dynamic::UniqueBias::sName(); }

    QString i18nDescription() const
    { return i18nc("Description of the unique bias",
                   "The unique bias adds matches tracks that are not already in the playlist effectively preventing duplicates."); }

    Dynamic::BiasPtr createBias()
    { return Dynamic::BiasPtr( new Dynamic::UniqueBias() ); }

    Dynamic::BiasPtr createBias( QXmlStreamReader *reader )
    { return Dynamic::BiasPtr( new Dynamic::UniqueBias( reader ) ); }
};


class NotBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const
    { return i18nc("Name of the \"Not\" bias", "Not"); }

    QString name() const
    { return Dynamic::NotBias::sName(); }

    QString i18nDescription() const
    { return i18nc("Description of the \"Not\" bias",
                   "The \"Not\" bias adds tracks that do not match any sub bias."); }

    Dynamic::BiasPtr createBias()
    { return Dynamic::BiasPtr( new Dynamic::NotBias() ); }

    Dynamic::BiasPtr createBias( QXmlStreamReader *reader )
    { return Dynamic::BiasPtr( new Dynamic::NotBias( reader ) ); }
};


class AndBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const
    { return i18nc("Name of the \"And\" bias", "And"); }

    QString name() const
    { return Dynamic::AndBias::sName(); }

    QString i18nDescription() const
    { return i18nc("Description of the \"And\" bias",
                   "The \"And\" bias adds tracks that match all of the sub biases at the same time."); }

    Dynamic::BiasPtr createBias()
    { return Dynamic::BiasPtr( new Dynamic::AndBias() ); }

    Dynamic::BiasPtr createBias( QXmlStreamReader *reader )
    { return Dynamic::BiasPtr( new Dynamic::AndBias( reader ) ); }
};


class OrBiasFactory : public Dynamic::AbstractBiasFactory
{
    QString i18nName() const
    { return i18nc("Name of the \"Or\" bias", "Or"); }

    QString name() const
    { return Dynamic::OrBias::sName(); }

    QString i18nDescription() const
    { return i18nc("Description of the \"Or\" bias",
                   "The \"Or\" bias adds tracks that match at least one of the sub biases at the same time."); }

    Dynamic::BiasPtr createBias()
    { return Dynamic::BiasPtr( new Dynamic::OrBias() ); }

    Dynamic::BiasPtr createBias( QXmlStreamReader *reader )
    { return Dynamic::BiasPtr( new Dynamic::OrBias( reader ) ); }
};

Dynamic::BiasFactory* Dynamic::BiasFactory::s_instance = 0;

QList<Dynamic::AbstractBiasFactory*> Dynamic::BiasFactory::s_biasFactories = QList<Dynamic::AbstractBiasFactory*>();

Dynamic::BiasFactory*
Dynamic::BiasFactory::instance()
{
    if( !s_instance )
    {
        // --- build in biases
        s_biasFactories.append( new Dynamic::TagMatchBiasFactory() );
        s_biasFactories.append( new Dynamic::PartBiasFactory() );
        s_biasFactories.append( new RandomBiasFactory() );
        s_biasFactories.append( new UniqueBiasFactory() );
        s_biasFactories.append( new NotBiasFactory() );
        s_biasFactories.append( new AndBiasFactory() );
        s_biasFactories.append( new OrBiasFactory() );
        s_biasFactories.append( new Dynamic::AlbumPlayBiasFactory() );
        s_biasFactories.append( new Dynamic::EchoNestBiasFactory() );

        s_instance = new BiasFactory( App::instance() );
    }
    return s_instance;
}

Dynamic::BiasFactory::BiasFactory( QObject *parent )
    : QObject( parent )
{ }

Dynamic::BiasPtr
Dynamic::BiasFactory::fromXml( QXmlStreamReader *reader )
{
    QStringRef name = reader->name();

    instance(); // ensure that we have an instance with the default factories
    foreach( Dynamic::AbstractBiasFactory* fac, s_biasFactories )
    {
        if( name == fac->name() )
            return fac->createBias( reader );
    }
    return BiasPtr();
}

Dynamic::BiasPtr
Dynamic::BiasFactory::fromName( const QString &name )
{
    instance(); // ensure that we have an instance with the default factories
    foreach( Dynamic::AbstractBiasFactory* fac, s_biasFactories )
    {
        if( name == fac->name() )
            return fac->createBias();
    }
    return BiasPtr();
}

void
Dynamic::BiasFactory::registerNewBiasFactory( Dynamic::AbstractBiasFactory* factory )
{
    instance(); // ensure that we have an instance with the default factories
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
    instance(); // ensure that we have an instance with the default factories
    return s_biasFactories;
}

void
Dynamic::BiasFactory::emitChanged()
{
    emit changed();
}

#include "BiasFactory.moc"
