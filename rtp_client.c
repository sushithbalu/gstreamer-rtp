#include <gst/gst.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *sink, *convert;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    /* Initialize GStreamer */
    gst_init (&argc, &argv);

    /* Create the elements */
    source = gst_element_factory_make ("v4l2src", "source");
    sink = gst_element_factory_make ("autovideosink", "sink");
    convert =gst_element_factory_make("videoconvert","convert");

    /* Create the empty pipeline */
    pipeline = gst_pipeline_new ("test-pipeline");

    if (!pipeline || !source || !sink || !convert) {
        g_printerr ("Not all elements could be created.\n");
        return -1;
    }

    /*set source as web cam*/
    g_object_set (source, "device", "/dev/video0", NULL);

    /* Build the pipeline */
    gst_bin_add_many (GST_BIN (pipeline), source, sink, convert, NULL);
    if (gst_element_link (convert, sink) != TRUE) {
        g_printerr ("Elements could not be linked confert sink.\n");
        gst_object_unref (pipeline);
        return -1;
    }


    if (gst_element_link (source, convert) != TRUE) {
        g_printerr ("Elements could not be linked source -convert.\n");
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
