// Author: Mark Kretschmann (C) Copyright 2004
// Copyright: See COPYING file that comes with this distribution

#include <plugin.h>

#include <kdebug.h>


amaroK::Plugin::Plugin( bool hasConfigure ) 
    : m_hasConfigure( hasConfigure )
{
    kdDebug() << k_funcinfo << endl;
}


amaroK::Plugin::~Plugin() 
{
    kdDebug() << k_funcinfo << endl;
}


