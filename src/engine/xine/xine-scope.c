/* Author: Max Howell <max.howell@methylblue.com>, (C) 2004
   Copyright: See COPYING file that comes with this distribution

   This has to be a c file or for some reason it won't link! (GCC 3.4.1)
*/

/*TODO determine how to compile -std=c99*/
#define inline
/* this define ABSOLUTELY ESSENTIAL for memcopy() to work in put_buffer() */
#define METRONOM_INTERNAL
#include <xine/xineutils.h>
#include <xine/post.h>

#define MAXCHANNELS  6


/* yes my own list structure! how typical */
typedef struct myNode_s myNode;
struct myNode_s
{
    myNode *next;
    audio_buffer_t buf;
};



typedef struct post_plugin_scope_s post_plugin_scope_t;
typedef struct post_class_scope_s post_class_scope_t;


myNode      myList;
int         myChannels;
metronom_t *myMetronom;
int64_t     current_vpts;
int         myMutex;


struct post_class_scope_s
{
    post_class_t post_class;

    xine_t      *xine;
};

struct post_plugin_scope_s
{
    post_plugin_t post;

    int channels;

    metronom_t *metronom;
};


static int
scope_port_open(xine_audio_port_t *port_gen, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode)
{
  post_audio_port_t   *port = (post_audio_port_t *)port_gen;
  post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;

  printf( "[xine-engine] post-plugin port_open()\n" );

  _x_post_rewire(&this->post);
  _x_post_inc_usage(port);

  port->stream = stream;
  port->bits = bits;
  port->rate = rate;
  port->mode = mode;

  this->channels = _x_ao_mode2channels(mode);
  if( this->channels > MAXCHANNELS ) this->channels = MAXCHANNELS;

  myChannels  = this->channels;
  myList.next = &myList; /* ths list is empty, I promise! */
  myMutex     = 0;

  return port->original_port->open(port->original_port, stream, bits, rate, mode );
}

static void
scope_port_close(xine_audio_port_t *port_gen, xine_stream_t *stream )
{
    post_audio_port_t   *port = (post_audio_port_t *)port_gen;
    post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;
    myNode *node, *next;


    printf( "[xine-engine] post-plugin port_close()\n" );

    port->stream = NULL;
    port->original_port->close(port->original_port, stream );
    this->metronom->set_master(this->metronom, NULL);

    for( node = myList.next; node->next != &myList; node = next )
    {
        next = node->next;

        free( node->buf.mem );
        free( node );
    }

    _x_post_dec_usage(port);
}

static void
scope_port_put_buffer( xine_audio_port_t *port_gen, audio_buffer_t *buf, xine_stream_t *stream )
{
    post_audio_port_t   *port = (post_audio_port_t *)port_gen;
    post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;
    myNode *node, *prev;


    /* MY GOD! Why do I have to do this!?
     * You see if I don't the metronom doesn't make the timestamps accurate! */
    memcpy( this->metronom, stream->metronom, sizeof( metronom_t ) );


    /*FIXME*/
    if( port->bits == 8 ) { printf( "You dare tempt me with 8 bits?!\n" ); return; }

    /*first we need to copy this buffer*/
    myNode *new_node = malloc( sizeof(myNode) );
    memcpy( &new_node->buf, buf, sizeof(audio_buffer_t) );

    new_node->buf.mem = malloc( buf->num_frames * this->channels * 2 );
    memcpy( new_node->buf.mem, buf->mem, buf->num_frames * this->channels * 2 );

    /* pass data to original port - TODO is this necessary? */
    port->original_port->put_buffer( port->original_port, buf, stream );

    /*now we need to prune the list of old buffers*/
    #if 0
    for( prev = &myList, node = myList.next; !myMutex && node->next != &myList; node = node->next )
    {
        buf = &node->buf;

        if( buf->stream == 0 && buf->vpts < current_vpts )
        {
            prev->next = node->next;

            free( buf->mem );
            free( node );

            node = prev;
        }

        prev = node;
    }
    #endif

    /*finally we should prepend the current buffer to the list*/
    new_node->next = myList.next;
    myList.next    = new_node;
}

static void
scope_dispose(post_plugin_t *this_gen)
{
    post_plugin_scope_t *this = (post_plugin_scope_t *)this_gen;

    if( _x_post_dispose( this_gen ) )
    {
        this->metronom->exit( this->metronom );

        free( this );
    }
}

/* plugin class functions */
static post_plugin_t*
scope_open_plugin( post_class_t *class_gen, int inputs, xine_audio_port_t **audio_target, xine_video_port_t **video_target )
{
    /*this function passes back a structure with pointers to all
    //the above functions*/

  post_class_scope_t  *class = (post_class_scope_t *)class_gen;
  post_plugin_scope_t *this  = (post_plugin_scope_t *)xine_xmalloc(sizeof(post_plugin_scope_t)); /*fills structure with zeros too*/
  post_in_t           *input;
  post_out_t          *output;
  post_audio_port_t   *port;

  _x_post_init( &this->post, 1, 0 );

  this->metronom = _x_metronom_init( 0, 1, class->xine );
  myMetronom = this->metronom;

  port = _x_post_intercept_audio_port( &this->post, audio_target[0], &input, &output );
  port->new_port.open       = scope_port_open;
  port->new_port.close      = scope_port_close;
  port->new_port.put_buffer = scope_port_put_buffer;

  this->post.xine_post.audio_input[0] = &port->new_port;

  this->post.dispose = scope_dispose;

  return &this->post;
}

static char*
scope_get_description( post_class_t *class_gen )
{
  return "scope xine-engine Scope";
}

static void
scope_class_dispose( post_class_t *class_gen )
{
  free( class_gen );
}

/* plugin class initialization function */
post_class_t*
scope_init_plugin( xine_t *xine )
{
    post_class_scope_t *class = (post_class_scope_t *)malloc(sizeof(post_class_scope_t));

    if (!class) return NULL;

    class->post_class.open_plugin     = scope_open_plugin;
    class->post_class.get_identifier  = scope_get_description;
    class->post_class.get_description = scope_get_description;
    class->post_class.dispose         = scope_class_dispose;

    class->xine                       = xine;

    return &class->post_class;
}
