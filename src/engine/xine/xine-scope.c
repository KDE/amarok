/* Author: Max Howell <max.howell@methylblue.com>, (C) 2004
   Copyright: See COPYING file that comes with this distribution

   This has to be a c file or for some reason it won't link! (GCC 3.4.1)
*/

/* gcc doesn't like inline for me */
#define inline

#include "xine-scope.h"
#include <xine/xineutils.h>
#include <xine/post.h>


static MyNode     theList;
static metronom_t theMetronom;

MyNode     *myList = &theList;
metronom_t *myMetronom = &theMetronom;
int         myChannels;


/*************************
 * post plugin functions *
 *************************/

static int
scope_port_open( xine_audio_port_t *port_gen, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode )
{
  post_audio_port_t *port = (post_audio_port_t*)port_gen;
  post_plugin_t     *this = (post_plugin_t *)port->post;

  _x_post_rewire( this );
  _x_post_inc_usage( port );

  port->stream = stream;
  port->bits = bits;
  port->rate = rate;
  port->mode = mode;

  myChannels = _x_ao_mode2channels( mode );

  return port->original_port->open( port->original_port, stream, bits, rate, mode );
}

static void
scope_port_close(xine_audio_port_t *port_gen, xine_stream_t *stream )
{
    post_audio_port_t *port = (post_audio_port_t*)port_gen;

    port->stream = NULL;
    port->original_port->close(port->original_port, stream );

    myList->next = myList;

    _x_post_dec_usage( port );
}

static void
scope_port_put_buffer( xine_audio_port_t *port_gen, audio_buffer_t *buf, xine_stream_t *stream )
{
    post_audio_port_t *port = (post_audio_port_t*)port_gen;

    /*FIXME*/
    if( port->bits == 8 ) { printf( "You dare tempt me with 8 bits?!\n" ); return; }

    /*first we need to copy this buffer*/
    MyNode *new_node = malloc( sizeof(MyNode) );
    memcpy( &new_node->buf, buf, sizeof(audio_buffer_t) );

    new_node->buf.mem = malloc( buf->num_frames * myChannels * 2 );
    memcpy( new_node->buf.mem, buf->mem, buf->num_frames * myChannels * 2 );

    /* pass data to original port - TODO is this necessary? */
    port->original_port->put_buffer( port->original_port, buf, stream );

    /* finally we should prepend the current buffer to the list
     * NOTE this _is_ thread-safe */
    new_node->next = theList.next;
    theList.next   = new_node;
}

static void
scope_dispose( post_plugin_t *this )
{
    free( this );
}


/************************
 * post class functions *
 ************************/

static post_plugin_t*
scope_class_open_plugin( post_class_t *class, int inputs, xine_audio_port_t **audio_target, xine_video_port_t **video_target )
{
    /* this function passes back a structure with pointers to all
       the above functions */

  post_plugin_t     *this = (post_plugin_t *)xine_xmalloc( sizeof(post_plugin_t) );
  post_in_t         *input;
  post_out_t        *output;
  post_audio_port_t *port;

  _x_post_init( this, 1, 0 );

  port = _x_post_intercept_audio_port( this, audio_target[0], &input, &output );
  port->new_port.open       = scope_port_open;
  port->new_port.close      = scope_port_close;
  port->new_port.put_buffer = scope_port_put_buffer;

  this->xine_post.audio_input[0] = &port->new_port;
  this->xine_post.type = PLUGIN_POST;

  this->dispose = scope_dispose;

  return this;
}

static void
scope_class_dispose( post_class_t *class_gen )
{
  free( class_gen );
}


/************************
 * plugin init function *
 ************************/

post_class_t*
scope_init_plugin( xine_t *xine )
{
    post_class_t *class = malloc( sizeof(post_class_t) );

    class->open_plugin     = scope_class_open_plugin;
    class->get_identifier  = NULL;
    class->get_description = NULL;
    class->dispose         = scope_class_dispose;

    return class;
}
