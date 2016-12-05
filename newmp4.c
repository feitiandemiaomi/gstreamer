/* 
To compile 

Build on : ubuntu 12.04 32 bit
gstreamer version installed: 1.45

gcc -Wall newmp4.c -o newmp4 -pthread -I/usr/local/include/gstreamer-1.0 -I/usr/include/glib-2.0 -I/usr/lib/i386-linux-gnu/glib-2.0/include  -L/usr/local/lib -lgstreamer-1.0 -lgobject-2.0 -lglib-2.0  

Builds Pipeline: 
gst-launch-1.0 -e filesrc location=/home/asuri/my_gstcode/SampleVideo_720x480_1mb.mp4 ! qtdemux name=mdemux mdemux.video_0 ! queue ! h264parse ! avdec_h264 ! videoconvert ! ximagesink mdemux.audio_0 ! queue ! faad ! audioconvert ! autoaudiosink

*/
#include <gst/gst.h>

typedef struct _CustomData {
    GstElement *pipeline;
    GstElement *source;
    GstElement *demuxer;
    GstElement *audioqueue;
    GstElement *videoqueue;
    GstElement *audio_decoder;
    GstElement *video_decoder;
    GstElement *video_convert;
    GstElement *audio_convert;
    GstElement *video_sink;
    GstElement *audio_sink;
} CustomData;


static gboolean my_bus_callback (GstBus *bus, GstMessage *msg, gpointer data);
static void pad_added_handler (GstElement *src, GstPad *pad, CustomData *data);

int main(int argc, char *argv[]) 
{
    GMainLoop *loop;
    CustomData data;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;
    gboolean terminate = FALSE;
    /* Initialize GStreamer */
    gst_init (&argc, &argv);
    loop = g_main_loop_new (NULL, FALSE);

    /* Create the elements */
    data.source = gst_element_factory_make ("filesrc", "source");
    data.demuxer = gst_element_factory_make ("qtdemux", "demuxer");
    data.audioqueue = gst_element_factory_make("queue","audioqueue");
    data.audio_decoder = gst_element_factory_make ("faad", "audio_decoder");
    data.audio_convert = gst_element_factory_make ("audioconvert", "audio_convert");
    data.audio_sink = gst_element_factory_make ("autoaudiosink", "audio_sink");
    data.videoqueue = gst_element_factory_make("queue","videoqueue");
    data.video_decoder = gst_element_factory_make("avdec_h264","video_decoder");
    data.video_convert = gst_element_factory_make("videoconvert","video_convert");
    data.video_sink = gst_element_factory_make("ximagesink","video_sink");
    data.pipeline = gst_pipeline_new ("test-pipeline");

    bus = gst_pipeline_get_bus (GST_PIPELINE (data.pipeline));
    gst_bus_add_watch (bus, my_bus_callback, loop);
    gst_object_unref (bus);

    if (!data.pipeline || !data.source || !data.demuxer || !data.audioqueue ||!data.audio_decoder ||!data.audio_convert ||
    !data.audio_sink || !data.videoqueue || !data.video_decoder || !data.video_convert || !data.video_sink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
    }

    gst_bin_add_many (GST_BIN (data.pipeline), data.source,data.demuxer,data.audioqueue,data.audio_decoder,data.audio_convert,data.audio_sink,data.videoqueue,data.video_decoder,data.video_convert,data.video_sink, NULL);

    if (!gst_element_link(data.source,data.demuxer)) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
    } 

    if (!gst_element_link_many (data.audioqueue,data.audio_decoder,data.audio_convert, data.audio_sink,NULL)) {
    g_printerr (" audio Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
    }

    if (!gst_element_link_many(data.videoqueue,data.video_decoder,data.video_convert, data.video_sink,NULL)) {
    g_printerr("video Elements could not be linked.\n");
    gst_object_unref(data.pipeline);
    return -1;
    } 

    g_print("Reached here\n");
    /* Set the file to play */
    g_object_set (data.source, "location", argv[1], NULL);


    g_signal_connect (data.demuxer, "pad-added", G_CALLBACK (pad_added_handler), &data);
    /* Start playing */
    ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
    g_main_loop_run (loop);

    if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
    }
    gst_object_unref (bus);
    gst_element_set_state (data.pipeline, GST_STATE_NULL);
    gst_object_unref (data.pipeline);
   
    return 0;
    }

/* This function will be called by the pad-added signal */
static void pad_added_handler (GstElement *src, GstPad *new_pad, CustomData *data) 
{
    GstPad *sink_pad_audio = gst_element_get_static_pad (data->audioqueue, "sink");
    GstPad *sink_pad_video = gst_element_get_static_pad (data->videoqueue, "sink");

    GstCaps *new_pad_caps;
    GstStructure *str;
    GstPad *videopad, *audiopad;
    const gchar *new_pad_type;
    GstPadLinkReturn ret;

    g_print("Entry in cb_newpad\n");
    g_print("Inside the pad_added_handler method \n");

    new_pad_caps = gst_pad_query_caps (new_pad, NULL);
    str = gst_caps_get_structure (new_pad_caps, 0);
    new_pad_type = gst_structure_get_name (str);

    if ( g_strrstr (gst_structure_get_name (str), "audio") != NULL )
    {
      ret = gst_pad_link (new_pad, sink_pad_audio);
      if (GST_PAD_LINK_FAILED (ret)) 
       { 
        g_print (" Type is '%s' but link failed.\n", new_pad_type);
       } 
       else 
      {
        g_print (" Link succeeded (type '%s').\n", new_pad_type);
      }
    } 
    else if ( g_strrstr (gst_structure_get_name (str), "video") != NULL )
    {
      ret = gst_pad_link (new_pad, sink_pad_video);

      if (GST_PAD_LINK_FAILED (ret)) 
      {
        g_print (" Type is '%s' but link failed.\n", new_pad_type);
      } 
      else 
      {
        g_print (" Link succeeded (type '%s').\n", new_pad_type);
      }
    } 


    else {
    g_print (" It has type '%s' which is not raw audio. Ignoring.\n", new_pad_type);
    goto exit;
    }
    exit:
    if (new_pad_caps != NULL)
    gst_caps_unref (new_pad_caps);
    gst_object_unref (sink_pad_audio);
    gst_object_unref (sink_pad_video);
}

static gboolean my_bus_callback (GstBus *bus, GstMessage *msg, gpointer data)
{
  GMainLoop *loop = (GMainLoop *) data;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

