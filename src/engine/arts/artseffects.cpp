/***************************************************************************
 file                 : artsengine.cpp - aRts audio interface
 begin                : Dec 31 2003
 copyright            : (C) 2003 Mark Kretschmann <markey@web.de>
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#if 0

void ArtsEngine::loadEffects()
{
    kdDebug() << k_funcinfo << endl;

    QDomDocument doc;
    QFile file( kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "arts-effects.xml" );

    if ( !file.open( IO_ReadOnly ) )
    {
        kdWarning() << "[ArtsEngine::loadEffects()] error: !file.open()" << endl;
        return;
    }

    QString errorMsg;
    int     errorLine;
    int     errorColumn;
    if ( !doc.setContent( &file, &errorMsg, &errorLine, &errorColumn ) )
    {
        kdWarning() << "[ArtsEngine::loadEffects()] error: !doc.setContent()" << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorMsg   : " << errorMsg    << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorLine  : " << errorLine   << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorColumn: " << errorColumn << endl;
        file.close();
        return;
    }

    QDomElement docElem = doc.documentElement();

    for ( QDomNode n = docElem.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        QString effect = n.namedItem( "effectname" ).firstChild().toText().nodeValue();
        kdDebug() << "effectname: " << effect << endl;

        long id = createEffect( effect );
        for ( QDomNode nAttr = n.firstChild(); id && !nAttr.isNull(); nAttr = nAttr.nextSibling() )
        {
            if ( nAttr.nodeName() == "attribute" )
            {
                QString name  = nAttr.namedItem( "name"  ).firstChild().toText().nodeValue();
                QString type  = nAttr.namedItem( "type"  ).firstChild().toText().nodeValue();
                QString value = nAttr.namedItem( "value" ).firstChild().toText().nodeValue();

                kdDebug() << "name : " << name  << endl;
                kdDebug() << "type : " << type  << endl;
                kdDebug() << "value: " << value << endl;

                Arts::DynamicRequest req( *m_effectMap[id].effect );
                std::string set( "_set_" );
                set.append( std::string( name.latin1() ) );
                req.method( set );

                Arts::Buffer buf;
                buf.fromString( std::string( value.latin1() ), "" );

                Arts::Any param;
                param.type = std::string( type.latin1() );
                param.readType( buf );
                req.param( param );

                if ( !req.invoke() )
                    kdWarning() << "DynamicRequest failed." << endl;
            }
        }
    }
}


void ArtsEngine::saveEffects()
{
    QDomDocument doc;
    QDomElement root = doc.createElement( "aRts-Effects" );
    doc.appendChild( root );
    QMap<long, EffectContainer>::Iterator end( m_effectMap.end() );
    for ( QMap<long, EffectContainer>::Iterator it = m_effectMap.begin(); it != end; ++it )
    {
        QDomElement tagEffect = doc.createElement( "effect" );
        root.appendChild( tagEffect );

            { //effectname
                QDomElement tag = doc.createElement( "effectname" );
                tagEffect.appendChild( tag );
                QDomText txt = doc.createTextNode( (*it.data().effect)._interfaceName().c_str() );
                tag.appendChild( txt );
            }

        Arts::InterfaceDef def = (*it.data().effect)._queryInterface( (*it.data().effect)._interfaceName() );

        for ( uint i = 0; i < def.attributes.size(); i++ )
        {
            QDomElement tagAttribute = doc.createElement( "attribute" );
            tagEffect.appendChild( tagAttribute );

            { //name
                QDomElement tag = doc.createElement( "name" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( def.attributes[i].name.c_str() );
                tag.appendChild( txt );
            }

            Arts::DynamicRequest req( *it.data().effect );
            req.method( "_get_" + def.attributes[i].name );
            Arts::Any result;
            result.type = def.attributes[i].type;

            { //type
                QDomElement tag = doc.createElement( "type" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( def.attributes[i].type.c_str() );
                tag.appendChild( txt );
            }

            if ( !req.invoke( result ) )
                kdWarning() << "request failed." << endl;

            Arts::Buffer buf;
            result.writeType( buf );

            { //value
                QDomElement tag = doc.createElement( "value" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( buf.toString( "" ).c_str() );
                tag.appendChild( txt );
            }
        }
        removeEffect( it.key() );
    }

    QString path = kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "arts-effects.xml";
    QFile::remove( path );
    QFile file( path );
    file.open( IO_ReadWrite );
    QTextStream stream( &file );
    stream << doc;
}


QStringList ArtsEngine::availableEffects() const
{
    QStringList val;
    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::StereoEffect" );
    query.supports( "Interface", "Arts::SynthModule" );
    std::vector<Arts::TraderOffer> *offers = query.query();

    for ( std::vector<Arts::TraderOffer>::iterator i = offers->begin(); i != offers->end(); ++i )
    {
        Arts::TraderOffer &offer = *i;
        QCString name = offer.interfaceName().c_str();
        val.append( name );
    }
    delete offers;

    return val;
}


std::vector<long> ArtsEngine::activeEffects() const
{
    std::vector<long> vec;
    QMap<long, EffectContainer>::ConstIterator it;
    QMap<long, EffectContainer>::ConstIterator end(m_effectMap.end() );
    for ( it = m_effectMap.begin(); it != end; ++it )
    {
        vec.push_back( it.key() );
    }

    return vec;
}


QString ArtsEngine::effectNameForId( long id ) const
{
    const std::string str = (*m_effectMap[id].effect)._interfaceName();
    QString qstr( str.c_str() );

    return qstr;
}


bool ArtsEngine::effectConfigurable( long id ) const
{
    if ( m_effectMap.find(id) == m_effectMap.end() )
        return false;

    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::GuiFactory" );
    query.supports( "CanCreate", (*m_effectMap[id].effect)._interfaceName() );

    std::vector<Arts::TraderOffer> *queryResults = query.query();
    bool yes = queryResults->size();
    delete queryResults;

    return yes;
}


long ArtsEngine::createEffect( const QString& name )
{
    const long error = 0;

    if ( name.isEmpty() )
        return error;

    Arts::StereoEffect* pFX = new Arts::StereoEffect;
    *pFX = Arts::DynamicCast( m_server.createObject( std::string( name.ascii() ) ) );

    if ( (*pFX).isNull() ) {
        kdWarning() << "[ArtsEngine::createEffect] error: could not create effect." << endl;
        delete pFX;
        return error;
    }

    pFX->start();
    long id = m_effectStack.insertBottom( *pFX, std::string( name.ascii() ) );

    if ( !id ) {
        kdWarning() << "[ArtsEngine::createEffect] error: insertBottom failed." << endl;
        pFX->stop();
        delete pFX;
        return error;
    }

    EffectContainer container;
    container.effect = pFX;
    container.widget = 0;

    m_effectMap.insert( id, container );

    return id;
}


void ArtsEngine::removeEffect( long id )
{
    m_effectStack.remove( id );

    m_effectMap[id].effect->stop();
    delete m_effectMap[id].widget;
    delete m_effectMap[id].effect;

    m_effectMap.remove( id );
}


void ArtsEngine::configureEffect( long id )
{
    if ( !m_effectMap[id].widget )
    {
        m_effectMap[id].widget = new ArtsConfigWidget( *m_effectMap[id].effect );
        m_effectMap[id].widget->show();
    }
}


// CLASS EffectConfigWidget --------------------------------------------------------

ArtsEngine::ArtsConfigWidget::ArtsConfigWidget( Arts::Object object )
        : QWidget( 0, 0, Qt::WType_TopLevel | Qt::WDestructiveClose )
{
    setCaption( kapp->makeStdCaption( QString( object._interfaceName().c_str() ) ) );

    Arts::GenericGuiFactory factory;
    m_gui = factory.createGui( object );

    if ( m_gui.isNull() )
    {
        kdWarning() << "Arts::Widget gui == NULL! Returning.." << endl;
        return;
    }

    else
    {
        m_pArtsWidget = new KArtsWidget( m_gui, this );

        QBoxLayout *lay = new QHBoxLayout( this );
        lay->add( m_pArtsWidget );
    }
}


ArtsEngine::ArtsConfigWidget::~ArtsConfigWidget()
{
    delete m_pArtsWidget;
    m_gui = Arts::Widget::null();
}


#endif

