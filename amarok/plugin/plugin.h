//
// Author: Mark Kretschmann (C) Copyright 2004
//
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMAROK_PLUGIN_H
#define AMAROK_PLUGIN_H

/**
 * Size doesn't matter!
 */

#define AMAROK_EXPORT_PLUGIN( classname ) \
    extern "C" { void* create_plugin() { return new classname; } } 
 
    
class Plugin
{
};


#endif /* AMAROK_PLUGIN_H */

