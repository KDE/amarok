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
    extern "C" { \
         classname* plugin_object; \
         void* create_plugin() { plugin_object = new classname; return plugin_object; } \
         void  delete_plugin() { delete plugin_object; } \
    } 
 
namespace amaroK {
    
class Plugin
{
    public:
        Plugin();
};

} //namespace amaroK


#endif /* AMAROK_PLUGIN_H */


