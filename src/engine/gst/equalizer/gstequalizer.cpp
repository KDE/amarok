// PCM time-domain equalizer
// (c) 2002 Felipe Rivera <liebremx at users sourceforge net>
// (c) 2004 Mark Kretschmann <markey@web.de>
// License: GPL V2

#include "config.h"

#include "gstequalizer.h"
#include "iir_cf.h"         // IIR filter coefficients

#include <string.h>
#include <kdebug.h>


GST_DEBUG_CATEGORY_STATIC ( gst_equalizer_debug );
#define GST_CAT_DEFAULT gst_equalizer_debug

/* signals and args */
enum {
    LAST_SIGNAL
};

enum {
    ARG_0
};

GstElementDetails gst_equalizer_details =
    GST_ELEMENT_DETAILS ( (gchar*) "Equalizer",
                          (gchar*) "Source",
                          (gchar*) "Parametric Equalizer",
                          (gchar*) "Mark Kretschmann <markey@web.de>" );

// static guint gst_equalizer_signals[ LAST_SIGNAL ] = { 0 };

#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_equalizer_debug, "equalizer", 0, "equalizer element");

GST_BOILERPLATE_FULL ( GstEqualizer, gst_equalizer, GstElement, (GTypeFlags) GST_TYPE_ELEMENT, _do_init );


/////////////////////////////////////////////////////////////////////////////////////
// INIT
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_equalizer_base_init ( gpointer g_class )
{
    kdDebug() << k_funcinfo << endl;

    GstElementClass * gstelement_class = GST_ELEMENT_CLASS ( g_class );
    gst_element_class_set_details ( gstelement_class, &gst_equalizer_details );
}


static void
gst_equalizer_class_init ( GstEqualizerClass * klass )
{
    kdDebug() << k_funcinfo << endl;

    GObjectClass* gobject_class;
    GstElementClass* gstelement_class = GST_ELEMENT_CLASS( klass );
    gobject_class = G_OBJECT_CLASS( klass );

//     g_object_class_install_property ( G_OBJECT_CLASS ( klass ), ARG_BLOCKSIZE,
//                                       g_param_spec_ulong ( "blocksize", "Block size",
//                                                            "Size in bytes to read per buffer", 1, G_MAXULONG, DEFAULT_BLOCKSIZE,
//                                                            ( GParamFlags ) G_PARAM_READWRITE ) );

    gobject_class->set_property = gst_equalizer_set_property;
    gobject_class->get_property = gst_equalizer_get_property;

    gstelement_class->change_state = gst_equalizer_change_state;
}


static void
gst_equalizer_init ( GstEqualizer* obj )
{
    kdDebug() << k_funcinfo << endl;

    obj->srcpad = gst_pad_new ( "src", GST_PAD_SRC );

    gst_pad_set_chain_function ( obj->srcpad, gst_equalizer_chain );
    gst_element_add_pad ( GST_ELEMENT ( obj ), obj->srcpad );

    // Properties
//     streamsrc->blocksize = DEFAULT_BLOCKSIZE;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

static void
gst_equalizer_set_property ( GObject * object, guint prop_id, const GValue * value,
                             GParamSpec * pspec )
{
    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_EQUALIZER ( object ) );

    GstEqualizer* obj = GST_EQUALIZER ( object );

    switch ( prop_id )
    {
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}


static void
gst_equalizer_get_property ( GObject * object, guint prop_id, GValue * value, GParamSpec * pspec )
{
    /* it's not null if we got it, but it might not be ours */
    g_return_if_fail ( GST_IS_EQUALIZER ( object ) );

    GstEqualizer* obj = GST_EQUALIZER ( object );

    switch ( prop_id )
    {
/*        case ARG_BLOCKSIZE:
            g_value_set_ulong ( value, obj->blocksize );
            break;*/
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID ( object, prop_id, pspec );
            break;
    }
}


static GstElementStateReturn
gst_equalizer_change_state (GstElement * element)
{
//   GstEqualizer *src = GST_EQUALIZER (element);

    switch (GST_STATE_TRANSITION (element))
    {
        case GST_STATE_NULL_TO_READY:
            break;
        case GST_STATE_READY_TO_NULL:
            break;
        case GST_STATE_READY_TO_PAUSED:
            break;
        case GST_STATE_PAUSED_TO_READY:
            break;
        default:
            break;
    }

    if (GST_ELEMENT_CLASS (parent_class)->change_state)
        return GST_ELEMENT_CLASS (parent_class)->change_state (element);

    return GST_STATE_SUCCESS;
}


static void clean_history( GstEqualizer* obj )
{
    /* Zero the history arrays */
    bzero(obj->data_history, sizeof(sXYData) * EQ_MAX_BANDS * EQ_CHANNELS);
}


static void
set_filters( GstEqualizer* obj, gint bands, gint sfreq )
{
    obj->rate = sfreq;
    switch(obj->rate)
    {
        case 11025: obj->iir_cf = iir_cf10_11k_11025;
                    obj->band_count = 10;
        break;
        case 22050: obj->iir_cf = iir_cf10_22k_22050;
                    obj->band_count = 10;
        break;
        case 48000:
            obj->band_count = BAND_NUM;
            switch( bands )
            {
                case 31: obj->iir_cf = iir_cf31_48000; break;
                case 25: obj->iir_cf = iir_cf25_48000; break;
                case 15: obj->iir_cf = iir_cf15_48000; break;
                default:
                         obj->iir_cf = iir_cf10_48000;
                break;
            }
        break;
        default:
            obj->band_count = BAND_NUM;
            obj->rate = 44100;
            switch( bands )
            {
                case 31: obj->iir_cf = iir_cf31_44100; break;
                case 25: obj->iir_cf = iir_cf25_44100; break;
                case 15: obj->iir_cf = iir_cf15_44100; break;
                default:
                         obj->iir_cf = iir_cf10_44100;
                break;
            }
        break;
    }
}


void
gst_equalizer_chain ( GstPad* pad, GstData* data_in )
{
//__inline__ int iir(gpointer * d, gint length, gint srate, gint nch)

    g_return_if_fail( pad != NULL );

    GstEqualizer* obj = GST_EQUALIZER ( GST_OBJECT_PARENT ( pad ) );
    GstBuffer* buf = GST_BUFFER( data_in );
    guint8* d = GST_BUFFER_DATA( buf );
    gint length = GST_BUFFER_SIZE( buf );
    gint srate = 41000;
    gint nch = 2;

    gint16 *data = (gint16*) *d;
    /* Indexes for the history arrays
     * These have to be kept between calls to this function
     * hence they are static */
    static gint i = 0, j = 2, k = 1;

    gint index, band, channel;
    gint tempgint, halflength;
    float out[EQ_CHANNELS], pcm[EQ_CHANNELS];

    // Load the correct filter table according to the sampling rate if needed
    if (srate != obj->rate)
    {
        set_filters( obj, BAND_NUM, srate );
        clean_history( obj );
    }

    /**
     * IIR filter equation is
     * y[n] = 2 * (alpha*(x[n]-x[n-2]) + gamma*y[n-1] - beta*y[n-2])
     *
     * NOTE: The 2 factor was introduced in the coefficients to save
     * 			a multiplication
     *
     * This algorithm cascades two filters to get nice filtering
     * at the expense of extra CPU cycles
     */
    /* 16bit, 2 bytes per sample, so divide by two the length of
     * the buffer (length is in bytes)
     */
    halflength = (length >> 1);
    for (index = 0; index < halflength; index+=2)
    {
        /* For each channel */
        for (channel = 0; channel < nch; channel++)
        {
            pcm[channel] = data[index+channel];
            /* Preamp gain */
            pcm[channel] *= obj->preamp[channel];

            out[channel] = 0.;
            /* For each band */
            for (band = 0; band < obj->band_count; band++)
            {
                /* Store Xi(n) */
                obj->data_history[band][channel].x[i] = pcm[channel];
                /* Calculate and store Yi(n) */
                obj->data_history[band][channel].y[i] =
                    (
                    /* = alpha * [x(n)-x(n-2)] */
                    obj->iir_cf[band].alpha * ( obj->data_history[band][channel].x[i]
                    -  obj->data_history[band][channel].x[k])
                    /* + gamma * y(n-1) */
                    + obj->iir_cf[band].gamma * obj->data_history[band][channel].y[j]
                    /* - beta * y(n-2) */
                    - obj->iir_cf[band].beta * obj->data_history[band][channel].y[k]
                    );
                /*
                 * The multiplication by 2.0 was 'moved' into the coefficients to save
                 * CPU cycles here */
                /* Apply the gain  */
                out[channel] +=  obj->data_history[band][channel].y[i]*obj->gain[band][channel]; // * 2.0;
            } /* For each band */

            /* Volume stuff
               Scale down original PCM sample and add it to the filters
               output. This substitutes the multiplication by 0.25
               Go back to use the floating point multiplication before the
               conversion to give more dynamic range
               */
            out[channel] += pcm[channel]*0.25;

            /* Round and convert to integer */
            tempgint = (int)out[channel];

            /* Limit the output */
            if (tempgint < -32768)
                data[index+channel] = -32768;
            else if (tempgint > 32767)
                data[index+channel] = 32767;
            else
                data[index+channel] = tempgint;
        } /* For each channel */

        i++; j++; k++;

        /* Wrap around the indexes */
        if (i == 3) i = 0;
        else if (j == 3) j = 0;
        else k = 0;
    }/* For each pair of samples */

    gst_pad_push( obj->srcpad, GST_DATA( buf ) );
}


GstEqualizer*
gst_equalizer_new ()
{
    GstEqualizer* obj = GST_EQUALIZER ( g_object_new ( GST_TYPE_EQUALIZER, NULL ) );
    gst_object_set_name( (GstObject*) obj, "Equalizer" );

//     object->m_buffering = buffering;

    return obj;
}


