/* Author: Max Howell <max.howell@methylblue.com>, (C) 2004
   Copyright: See COPYING file that comes with this distribution

   This has to be a c file or for some reason it won't link! (GCC 3.4.1)
*/

/* gcc doesn't like inline for me */
#define inline
/* need access to port_ticket */
#define XINE_ENGINE_INTERNAL

#include "xine-scope.h"
#include <xine/post.h>
#include <xine/xine_internal.h>


static MyNode     theList;
static metronom_t theMetronom;

MyNode     *myList     = &theList;
metronom_t *myMetronom = &theMetronom;
int         myChannels = 2;


/*************************
 * post plugin functions *
 *************************/

static int
scope_port_open( xine_audio_port_t *port_gen, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode )
{
    #define port ((post_audio_port_t*)port_gen)

    _x_post_rewire( (post_plugin_t*)port->post );
    _x_post_inc_usage( port );

    port->stream = stream;
    port->bits = bits;
    port->rate = rate;
    port->mode = mode;

    myChannels = _x_ao_mode2channels( mode );

    return port->original_port->open( port->original_port, stream, bits, rate, mode );
}

static void
scope_port_close( xine_audio_port_t *port_gen, xine_stream_t *stream )
{
    port->stream = NULL;
    port->original_port->close( port->original_port, stream );

    myList->next = myList;

    _x_post_dec_usage( port );
}

static void
scope_port_put_buffer( xine_audio_port_t *port_gen, audio_buffer_t *buf, xine_stream_t *stream )
{
    const int num_samples = buf->num_frames * myChannels;

    /*FIXME both these please*/
    if( port->bits == 8 ) {
       printf( "You dare tempt me with 8 bits?!\n" ); return; }
    if( buf->stream == 0 ) {
       port->original_port->put_buffer( port->original_port, buf, stream );
       /*printf( "stream == 0! what does that mean?!\n" );*/ return; }

    /* I keep my own metronom because xine wouldn't for some reason */
    memcpy( myMetronom, stream->metronom, sizeof(metronom_t) );

    MyNode *new_node     = malloc( sizeof(MyNode) );
    new_node->vpts       = myMetronom->got_audio_samples( myMetronom, buf->vpts, buf->num_frames );
    new_node->num_frames = buf->num_frames;
    new_node->mem        = malloc( num_samples * 2 );
    memcpy( new_node->mem, buf->mem, num_samples * 2 );

    {
        int64_t
        K  = myMetronom->pts_per_smpls; /*smpls = 1<<16 samples*/
        K *= num_samples;
        K /= (1<<16);
        K += new_node->vpts;

        new_node->vpts_end = K;
    }

    /* pass data to original port - TODO is this necessary? */
    port->original_port->put_buffer( port->original_port, buf, stream );

    /* finally we should append the current buffer to the list
     * NOTE this is thread-safe due to the way we handle the list in the GUI thread */
    new_node->next = myList->next;
    myList->next   = new_node;

    #undef port
}

static void
scope_dispose( post_plugin_t *this )
{
    free( this );
}


/************************
 * plugin init function *
 ************************/

xine_post_t*
scope_plugin_new( xine_t *xine, xine_audio_port_t *audio_target )
{
    post_plugin_t *post_plugin = xine_xmalloc( sizeof(post_plugin_t) );

    {
        post_plugin_t     *this = post_plugin;
        post_in_t         *input;
        post_out_t        *output;
        post_audio_port_t *port;

        _x_post_init( this, 1, 0 );

        port = _x_post_intercept_audio_port( this, audio_target, &input, &output );
        port->new_port.open       = scope_port_open;
        port->new_port.close      = scope_port_close;
        port->new_port.put_buffer = scope_port_put_buffer;

        this->xine_post.audio_input[0] = &port->new_port;
        this->xine_post.type = PLUGIN_POST;

        this->dispose = scope_dispose;
    }

    /* code is straight from xine_init_post()
       can't use that function as it only dlopens the plugins
       and our plugin is statically linked in */

    post_plugin->running_ticket = xine->port_ticket;
    post_plugin->xine = xine;
  /*post_plugin->node = NULL;*/

    /*
    xine_post_in_t *input = xine_list_first_content( post_plugin->input );

    post_plugin->input_ids    = malloc( sizeof(char*) * 2 );
    post_plugin->input_ids[0] = input->name;
    post_plugin->input_ids[1] = NULL;

    post_plugin->output_ids    = malloc( sizeof(char*) );
    post_plugin->output_ids[0] = NULL;
    */

    return &post_plugin->xine_post;
}
