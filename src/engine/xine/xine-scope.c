/* Author: Max Howell <max.howell@methylblue.com>, (C) 2004
   Copyright: See COPYING file that comes with this distribution

   This has to be a c file or for some reason it won't link! (GCC 3.4.1)
*/

/*TODO determine how to compile -std=c99*/
#define inline

#include <xine/xineutils.h>
#include <xine/post.h>

#define NUMSAMPLES 512
#define MAXCHANNELS  6


typedef struct post_plugin_scope_s post_plugin_scope_t;
typedef struct post_class_scope_s post_class_scope_t;

short myBuffer[MAXCHANNELS][NUMSAMPLES];


struct post_class_scope_s
{
    post_class_t post_class;

    xine_t      *xine;
};

struct post_plugin_scope_s
{
    post_plugin_t post;

    int data_idx;
    audio_buffer_t buf;   /* dummy buffer just to hold a copy of audio data */

    int channels;
    int samples_left;     /* the number of samples we have left to put in myBuffer */
};


static int
scope_port_open(xine_audio_port_t *port_gen, xine_stream_t *stream, uint32_t bits, uint32_t rate, int mode)
{
  post_audio_port_t    *port = (post_audio_port_t *)port_gen;
  post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;

  printf( "port_open!\n" );

  _x_post_rewire(&this->post);
  _x_post_inc_usage(port);

  port->stream = stream;
  port->bits = bits;
  port->rate = rate;
  port->mode = mode;

  this->channels = _x_ao_mode2channels(mode);
  if( this->channels > MAXCHANNELS ) this->channels = MAXCHANNELS;
  this->data_idx = 0;

  return port->original_port->open(port->original_port, stream, bits, rate, mode );
}

static void
scope_port_close(xine_audio_port_t *port_gen, xine_stream_t *stream )
{
    post_audio_port_t    *port = (post_audio_port_t *)port_gen;
    post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;

    printf( "port_close!\n" );

    port->stream = NULL;

    port->original_port->close(port->original_port, stream );

    _x_post_dec_usage(port);
}

static void
scope_port_put_buffer (xine_audio_port_t *port_gen, audio_buffer_t *buf, xine_stream_t *stream)
{
    post_audio_port_t    *port = (post_audio_port_t *)port_gen;
    post_plugin_scope_t *this = (post_plugin_scope_t *)port->post;

    int samples_used = 0;

    /* make a copy of buf data for private use */
    if( this->buf.mem_size < buf->mem_size )
    {
        this->buf.mem = (int16_t*)realloc(this->buf.mem, buf->mem_size);
        this->buf.mem_size = buf->mem_size;
    }

    memcpy( this->buf.mem, buf->mem, buf->num_frames * this->channels * ((port->bits == 8)?1:2) ); /*TODO bitshift instead of branch*/
    this->buf.num_frames = buf->num_frames;

    /* pass data to original port */
    port->original_port->put_buffer(port->original_port, buf, stream );

    /* we must not use original data anymore, it should have already being moved
    * to the fifo of free audio buffers. just use our private copy instead.
    */
    buf = &this->buf;

    this->samples_left += buf->num_frames;

    do
    {
        if( port->bits == 8 )
        {
            int c, i;
            int8_t*
            data8  = (int8_t *)buf->mem;
            data8 += samples_used * this->channels;

            /* scale 8 bit data to 16 bits and convert to signed as well */
            for( i = 0; i < buf->num_frames && this->data_idx < NUMSAMPLES; i++, this->data_idx++, data8 += this->channels )
                for( c = 0; c < this->channels; c++)
                    myBuffer[c][this->data_idx] = ((int16_t)data8[c] << 8) - 0x8000;
        } else {
            int c, i;
            int16_t*
            data16  = buf->mem;
            data16 += samples_used * this->channels;

            for( i = 0; i < buf->num_frames && this->data_idx < NUMSAMPLES; i++, this->data_idx++, data16 += this->channels )
                for( c = 0; c < this->channels; c++)
                    myBuffer[c][this->data_idx] = data16[c];
        }

        if( this->data_idx == NUMSAMPLES )
        {
            this->data_idx = 0;
            samples_used += NUMSAMPLES;
            this->samples_left -= NUMSAMPLES;
        }
    }
    while( this->samples_left >= NUMSAMPLES );
}

static void
scope_dispose(post_plugin_t *this_gen)
{
    post_plugin_scope_t *this = (post_plugin_scope_t *)this_gen;

    if( _x_post_dispose( this_gen ) )
    {
        if( this->buf.mem ) free( this->buf.mem );
        free( this );
    }
}

/* plugin class functions */
static post_plugin_t*
scope_open_plugin( post_class_t *class_gen, int inputs, xine_audio_port_t **audio_target, xine_video_port_t **video_target )
{
    /*this function passes back a structure with pointers to all
    //the above functions*/

  printf( "open_plugin\n" );

  post_plugin_scope_t *this  = (post_plugin_scope_t *)xine_xmalloc(sizeof(post_plugin_scope_t)); /*fills structure with zeros too*/
  post_in_t            *input;
  post_out_t           *output;
  post_audio_port_t    *port;

  _x_post_init( &this->post, 1, 0 );

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
    printf( "init_plugin\n" );

    post_class_scope_t *class = (post_class_scope_t *)malloc(sizeof(post_class_scope_t));

    if (!class) return NULL;

    class->post_class.open_plugin     = scope_open_plugin;
    class->post_class.get_identifier  = scope_get_description;
    class->post_class.get_description = scope_get_description;
    class->post_class.dispose         = scope_class_dispose;

    class->xine                       = xine;

    return &class->post_class;
}
