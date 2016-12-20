#include <gst/gst.h>
#include <glib.h>
#include <stdlib.h>
#define EXIT_NOT_ENOUGH_PARAMETERS -1
#define EXIT_ELEMENT_CREATION_FAILURE -2

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
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

static void on_pad_added (GstElement *element,GstPad *pad,gpointer data)
{
	GstPad *sinkpad;
	GstElement *s = (GstElement *) data;
	/* pad for decodebin */
	g_print ("Dynamic pad created, linking decoder/sink\n");
	sinkpad = gst_element_get_static_pad (s, "sink");
	gst_pad_link (pad, sinkpad);
	gst_object_unref (sinkpad);

}

int main(int argc, char *argv[]) {

	GstElement *pipeline, *source, *videosink, *audiosink, *audioconvert, *videoconvert, *rtpdepay;
	GstElement *decodebin, *queue, *queue1;
	GstStateChangeReturn ret;
	GstCaps *src_caps;
	GstBus *bus;
	GMainLoop *loop;
	guint bus_watch_id;

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	loop = g_main_loop_new (NULL, FALSE);

	if( argc != 2) {
		g_printerr ("Usage: %s <port>\n", argv[0]);
		exit(EXIT_NOT_ENOUGH_PARAMETERS);
	}

	/**
	gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp, media=(string)video, 
	clock-rate=(int)90000, encoding-name=(string)MP2T, payload=(int)33" ! rtpmp2tdepay ! 
	decodebin name=decoder ! queue ! audioconvert ! autoaudiosink decoder. ! queue ! 
	videoconvert ! autovideosink
	**/

	/* Create the elements */
	source = gst_element_factory_make ("udpsrc", "source");
	rtpdepay = gst_element_factory_make("rtpmp2tdepay", "video_depay");	
	decodebin = gst_element_factory_make("decodebin", "decodebin");
	queue = gst_element_factory_make("queue", "queue");
	audioconvert = gst_element_factory_make("audioconvert", "audio_converter");
	audiosink = gst_element_factory_make("autoaudiosink", "audio_sink");
	queue1 = gst_element_factory_make("queue", "queue1");
	videoconvert = gst_element_factory_make("videoconvert", "video_converter");
	videosink = gst_element_factory_make("autovideosink", "video_sink");

	/* Create the empty pipeline */
	pipeline = gst_pipeline_new ("rtp-pipeline");

	if (!pipeline || !source || !queue || !rtpdepay || !queue1 || !decodebin ||
		 !audioconvert || !audiosink || !videoconvert || !videosink ) {
		g_printerr ("Not all elements could be created.\n");
		exit(EXIT_ELEMENT_CREATION_FAILURE);
	}
	
	src_caps =  gst_caps_new_simple ("application/x-rtp","media", G_TYPE_STRING, "video", 
			"clock-rate", G_TYPE_INT, 90000, "encoding-name", G_TYPE_STRING, "MP2T",
			"payload", G_TYPE_INT, 33,  NULL);

	/*set rtp stream sink*/
	g_object_set (G_OBJECT (source), "port", g_ascii_strtoll(argv[1],NULL, 10), NULL);
	g_object_set (G_OBJECT (source), "caps", src_caps , NULL);
	gst_caps_unref(src_caps);

	/* we add a message handler */
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);

	/* Build the pipeline */
	gst_bin_add_many (GST_BIN (pipeline), source, rtpdepay, decodebin, queue, audioconvert, 
		audiosink, queue1, videoconvert, videosink, NULL);

	if (!gst_element_link_many (source, rtpdepay, decodebin, NULL)) {
		g_printerr ("Elements could not be linked convert.\n");
		gst_object_unref (pipeline);
		return -1;
	}
	if (!gst_element_link_many (queue, audioconvert, audiosink, NULL)) {
		g_printerr ("Elements could not be linked convert.\n");
		gst_object_unref (pipeline);
		return -1;
	}
	g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), queue);

	if (!gst_element_link_many (queue1, videoconvert, videosink, NULL)) {
		g_printerr ("Elements could not be linked confert video encode, mux.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	
	g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), queue1);
	/* Start playing */
	ret = gst_element_set_state (pipeline, GST_STATE_PLAYING);
	if (ret == GST_STATE_CHANGE_FAILURE) {
		g_printerr ("Unable to set the pipeline to the playing state.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	g_print ("Running...\n");
	g_main_loop_run (loop);

	g_print ("Returned, stopping playback\n");
	gst_element_set_state (pipeline, GST_STATE_NULL);

	gst_object_unref (GST_OBJECT(pipeline));
	g_source_remove (bus_watch_id);
	g_main_loop_unref (loop);

	return 0;
}
