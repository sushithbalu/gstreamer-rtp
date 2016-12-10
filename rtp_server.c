

#include <gst/gst.h>
#include <glib.h>
#include <stdio.h>

#define RTP_PORT 5000

int main(int argc, char *argv[]) {
	GstElement *pipeline, *source, *sink, *convert, *video_encoder, *rtppay;
	GstElement *queue, *queue1, *queue2, *queue3, *mic_src, *audio_resample;
	GstElement *audio_convert, *audio_encode, *av_mux;
	GstBus *bus;
	GstMessage *msg;
	GstStateChangeReturn ret;
	GstCaps *caps;

	/* Initialize GStreamer */
	gst_init (&argc, &argv);

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
	pipeline = gst_pipeline_new ("test-pipeline");

	if (!pipeline || !source || !sink || !convert 
		|| !queue || !queue1 || !queue2 || !queue3 || 
		!video_encoder || !rtppay|| !mic_src || 
		!av_mux || !audio_encode || !audio_resample|| !audio_convert) {
		g_printerr ("Not all elements could be created.\n");
		return -1;
	}
 
	/*set source as web cam*/
	g_object_set (source, "device", "/dev/video0", NULL);

	g_object_set(G_OBJECT(video_encoder),"tune", 4,"speed-preset", 3, NULL);
	//"threads", 1,"bframes", 0, "b-adapt", 0,"cabac", 0,"dct8x8", 0,"aud", 0,
	//"byte-stream", 1,"key-int-max", 10,"quantizer", 50,"vbv-buf-capacity", 0,NULL);

	/*set rtp stream sink*/
	g_object_set (G_OBJECT (sink), "port", RTP_PORT, NULL);
	g_object_set (G_OBJECT (sink), "host", argv[1], NULL);
	g_object_set (G_OBJECT (sink), "sync", FALSE, NULL);

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
/*
	 gst-launch-1.0 -v  v4l2src device=/dev/video0 ! videoconvert ! queue !
	 x264enc tune=zerolatency speed-preset=superfast ! queue ! mux2. pulsesrc ! 
	 audioresample ! audioconvert ! queue ! faac ! queue ! mux2. mpegtsmux name="mux2" !
	 rtpmp2tpay ! udpsink host=192.168.1.101 port=5000 sync=false auto-multicast=true
*/

	if (!gst_element_link_many ( mic_src, audio_convert, audio_resample , queue2, NULL)) {
		g_printerr ("Elements could not be linked confert audio queue\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link (queue2, audio_encode)) {
		g_printerr ("Elements could not be linked confert queue2, mux.\n");
		gst_object_unref (pipeline);
		return -1;
	}

	if (!gst_element_link_many (audio_encode, queue3, av_mux, NULL)) {
		g_printerr ("Elements could not be linked confert queue3.\n");
		gst_object_unref (pipeline);
		return -1;
	}
	
	caps = gst_caps_new_simple ("video/mpegts", "mpegversion", G_TYPE_INT, 1, NULL);
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

	/* Wait until error or EOS */
	bus = gst_element_get_bus (pipeline);
	msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

	/* Parse message */
	if (msg != NULL) {
		GError *err;
		gchar *debug_info;

		switch (GST_MESSAGE_TYPE (msg)) {
			case GST_MESSAGE_ERROR:
				gst_message_parse_error (msg, &err, &debug_info);
				g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
				g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
				g_clear_error (&err);
				g_free (debug_info);
				break;
			case GST_MESSAGE_EOS:
				g_print ("End-Of-Stream reached.\n");
				break;
			default:
				/* We should not reach here because we only asked for ERRORs and EOS */
				g_printerr ("Unexpected message received.\n");
				break;
		}
		gst_message_unref (msg);
	}

	/* Free resources */
	gst_object_unref (bus);
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (pipeline);
	return 0;
}
