#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

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

int main(int argc, char *argv[]) {

	GstElement *pipeline, *source, *sink, *convert, *video_encoder, *rtppay;
	GstElement *queue, *queue1, *queue2, *queue3, *mic_src, *audio_resample;
	GstElement *audio_convert, *audio_encode, *av_mux;
	GstStateChangeReturn ret;
	GstCaps *caps;
	GstBus *bus;
	GMainLoop *loop;
	guint bus_watch_id;

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

	loop = g_main_loop_new (NULL, FALSE);

	if( argc < 3) {
		g_printerr ("Usage: %s <host>  <port>\n", argv[0]);
		return -1;
	}

	/**
	  gst-launch-1.0 -v  v4l2src device=/dev/video0 ! videoconvert ! queue !
	  x264enc tune=zerolatency speed-preset=superfast ! queue ! mux2. pulsesrc ! 
	  audioresample ! audioconvert ! queue ! faac ! queue ! mux2. mpegtsmux name="mux2" !
	  rtpmp2tpay ! udpsink host=192.168.1.101 port=5000 sync=false auto-multicast=true

	 **/

	/* Create the elements */
	source = gst_element_factory_make ("v4l2src", "source");
	convert =gst_element_factory_make("videoconvert","convert");
	queue = gst_element_factory_make("queue", "queue");
	video_encoder = gst_element_factory_make("x264enc", "video_encoder");
	queue1 = gst_element_factory_make("queue", "queue1");
	mic_src = gst_element_factory_make("pulsesrc", "audio");
	audio_resample = gst_element_factory_make("audioresample", "audio_resample");
	audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
	queue2 = gst_element_factory_make("queue", "queue2");
	audio_encode = gst_element_factory_make("faac", "aac_conv");
	queue3 = gst_element_factory_make("queue", "queue3");
	av_mux = gst_element_factory_make("mpegtsmux", "mux");
	rtppay = gst_element_factory_make("rtpmp2tpay", "video_pay");	
	sink = gst_element_factory_make ("udpsink", "net-output");

	/* Create the empty pipeline */
	pipeline = gst_pipeline_new ("rtp-pipeline");

	if (!pipeline || !source || !sink || !convert || !queue || !queue1 || 
			!queue2 || !queue3 ||!video_encoder || !rtppay || !mic_src || 
			!av_mux || !audio_encode || !audio_resample || !audio_convert) {
		g_printerr ("Not all elements could be created.\n");
		return -1;
	}

	/*set source as web cam*/
	g_object_set (source, "device", "/dev/video0", NULL);


	g_object_set(G_OBJECT(video_encoder),"tune", 4,"speed-preset", 5,// NULL );
	"threads", 1,"bframes", 0, "b-adapt", 0,"cabac", 0,"dct8x8", 0,"aud", 0,
	"byte-stream", 1,"key-int-max", 10,"quantizer", 50,"vbv-buf-capacity", 0,NULL);

	/*set rtp stream sink*/
	g_object_set (G_OBJECT (sink), "port", atoi(argv[2]), NULL);
	g_object_set (G_OBJECT (sink), "host", argv[1], NULL);
	g_object_set (G_OBJECT (sink), "sync", FALSE, NULL);
	g_object_set (G_OBJECT (sink), "auto-multicast", TRUE, NULL);

	/* we add a message handler */
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	bus_watch_id = gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);
	
	/* Build the pipeline */
	gst_bin_add_many (GST_BIN (pipeline), source, convert, 
			queue, video_encoder, queue1, mic_src, audio_resample,
			audio_convert, queue2, audio_encode, queue3, av_mux,
			rtppay, sink, NULL);

	if (!gst_element_link (source, convert)) {
		g_printerr ("Elements could not be linked source -convert.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link_many (convert, queue, video_encoder, queue1, av_mux, NULL)) {
		g_printerr ("Elements could not be linked confert video encode, mux.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link_many ( mic_src, audio_convert, audio_resample , queue2, NULL)) {
		g_printerr ("Elements could not be linked confert audio queue\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link_many (queue2, audio_encode, queue3, av_mux, NULL)) {
		g_printerr ("Elements could not be linked confert queue3.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	caps = gst_caps_new_simple ("video/mpegts", "mpegversion", G_TYPE_INT, 2, NULL);
	if (!gst_element_link_filtered (av_mux, rtppay, caps)) {
		g_printerr ("Elements could not be linked confert vide.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link (rtppay, sink)) {
		g_printerr ("Elements could not be linked confert sink.\n");
		gst_object_unref (pipeline);
		return -1;
	}


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
