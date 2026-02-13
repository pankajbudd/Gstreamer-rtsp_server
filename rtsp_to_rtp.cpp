#include <gst/gst.h>
#include <stdio.h>

typedef struct {
    GstElement *pipeline;
    GstElement *rtspsrc;
    GstElement *video_queue, *video_depay, *video_dec, *video_convert;
    GstElement *video_enc, *video_pay, *video_sink;
    GstElement *audio_queue, *audio_depay, *audio_dec, *audio_convert;
    GstElement *audio_resample, *audio_enc, *audio_pay, *audio_sink;
    GMainLoop *loop;
} PipelineData;

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    PipelineData *pdata = (PipelineData *)data;
    
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            g_main_loop_quit(pdata->loop);
            break;
        }
        case GST_MESSAGE_EOS: {
            g_print("End of stream - Restarting RTSP session...\n");
            // Stop the pipeline
            gst_element_set_state(pdata->pipeline, GST_STATE_NULL);
            // Wait a moment for cleanup
            g_usleep(500000); // 500ms
            // Restart the pipeline
            gst_element_set_state(pdata->pipeline, GST_STATE_PLAYING);
            g_print("RTSP session restarted\n");
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(pdata->pipeline)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                g_print("Pipeline state changed from %s to %s\n",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
            }
            break;
        default:
            break;
    }
    return TRUE;
}

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    PipelineData *pdata = (PipelineData *)data;
    GstPad *sinkpad;
    GstCaps *caps;
    GstStructure *str;
    const gchar *media_type;
    
    caps = gst_pad_get_current_caps(pad);
    str = gst_caps_get_structure(caps, 0);
    media_type = gst_structure_get_string(str, "media");
    
    if (g_strcmp0(media_type, "video") == 0) {
        g_print("Linking video pad\n");
        sinkpad = gst_element_get_static_pad(pdata->video_queue, "sink");
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr("Failed to link video pad\n");
        }
        gst_object_unref(sinkpad);
    } else if (g_strcmp0(media_type, "audio") == 0) {
        g_print("Linking audio pad\n");
        sinkpad = gst_element_get_static_pad(pdata->audio_queue, "sink");
        if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
            g_printerr("Failed to link audio pad\n");
        }
        gst_object_unref(sinkpad);
    }
    
    gst_caps_unref(caps);
}

int main(int argc, char *argv[]) {
    PipelineData data;
    GstBus *bus;
    GstCaps *video_caps, *audio_caps;
    
    gst_init(&argc, &argv);
    
    // Create pipeline
    data.pipeline = gst_pipeline_new("rtsp-to-udp");
    
    // Create elements
    data.rtspsrc = gst_element_factory_make("rtspsrc", "src");
    
    // Video elements
    data.video_queue = gst_element_factory_make("queue", "video_queue");
    data.video_depay = gst_element_factory_make("rtpvp8depay", "video_depay");
    data.video_dec = gst_element_factory_make("avdec_vp8", "video_dec");
    data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
    data.video_enc = gst_element_factory_make("openh264enc", "video_enc");
    data.video_pay = gst_element_factory_make("rtph264pay", "video_pay");
    data.video_sink = gst_element_factory_make("udpsink", "video_sink");
    
    // Audio elements
    data.audio_queue = gst_element_factory_make("queue", "audio_queue");
    data.audio_depay = gst_element_factory_make("rtpopusdepay", "audio_depay");
    data.audio_dec = gst_element_factory_make("opusdec", "audio_dec");
    data.audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
    data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
    data.audio_enc = gst_element_factory_make("opusenc", "audio_enc");
    data.audio_pay = gst_element_factory_make("rtpopuspay", "audio_pay");
    data.audio_sink = gst_element_factory_make("udpsink", "audio_sink");
    
    // Check if all elements were created
    if (!data.pipeline || !data.rtspsrc || !data.video_queue || !data.video_depay ||
        !data.video_dec || !data.video_convert || !data.video_enc || !data.video_pay ||
        !data.video_sink || !data.audio_queue || !data.audio_depay || !data.audio_dec ||
        !data.audio_convert || !data.audio_resample || !data.audio_enc || 
        !data.audio_pay || !data.audio_sink) {
        g_printerr("Failed to create elements\n");
        return -1;
    }
    
    // Configure rtspsrc
    g_object_set(data.rtspsrc, 
        "location", "rtsp://127.0.0.1:8554/test",
        "latency", 200,
        NULL);
    
    // Configure video encoder
    g_object_set(data.video_enc,
        "bitrate", 2000,
        NULL);
    
    // Configure video payloader
    g_object_set(data.video_pay,
        "config-interval", 1,
        "pt", 96,
        NULL);
    
    // Configure video udpsink
    g_object_set(data.video_sink,
        "host", "127.0.0.1",
        "port", 5000,
        "sync", TRUE,
        NULL);
    
    // Configure audio payloader
    g_object_set(data.audio_pay,
        "pt", 97,
        NULL);
    
    // Configure audio udpsink
    g_object_set(data.audio_sink,
        "host", "127.0.0.1",
        "port", 5002,
        "sync", TRUE,
        NULL);
    
    // Add elements to pipeline
    gst_bin_add_many(GST_BIN(data.pipeline),
        data.rtspsrc,
        data.video_queue, data.video_depay, data.video_dec, data.video_convert,
        data.video_enc, data.video_pay, data.video_sink,
        data.audio_queue, data.audio_depay, data.audio_dec, data.audio_convert,
        data.audio_resample, data.audio_enc, data.audio_pay, data.audio_sink,
        NULL);
    
    // Create caps for video
    video_caps = gst_caps_new_simple("application/x-rtp",
        "media", G_TYPE_STRING, "video",
        "encoding-name", G_TYPE_STRING, "VP8",
        "payload", G_TYPE_INT, 96,
        NULL);
    
    // Create caps for audio
    audio_caps = gst_caps_new_simple("application/x-rtp",
        "media", G_TYPE_STRING, "audio",
        "encoding-name", G_TYPE_STRING, "OPUS",
        "payload", G_TYPE_INT, 97,
        "clock-rate", G_TYPE_INT, 48000,
        NULL);
    
    // Link video elements
    if (!gst_element_link_filtered(data.video_queue, data.video_depay, video_caps)) {
        g_printerr("Failed to link video queue to depay\n");
        gst_caps_unref(video_caps);
        gst_caps_unref(audio_caps);
        return -1;
    }
    
    if (!gst_element_link_many(data.video_depay, data.video_dec, data.video_convert,
        data.video_enc, data.video_pay, data.video_sink, NULL)) {
        g_printerr("Failed to link video elements\n");
        gst_caps_unref(video_caps);
        gst_caps_unref(audio_caps);
        return -1;
    }
    
    // Link audio elements
    if (!gst_element_link_filtered(data.audio_queue, data.audio_depay, audio_caps)) {
        g_printerr("Failed to link audio queue to depay\n");
        gst_caps_unref(video_caps);
        gst_caps_unref(audio_caps);
        return -1;
    }
    
    if (!gst_element_link_many(data.audio_depay, data.audio_dec, data.audio_convert,
        data.audio_resample, data.audio_enc, data.audio_pay, data.audio_sink, NULL)) {
        g_printerr("Failed to link audio elements\n");
        gst_caps_unref(video_caps);
        gst_caps_unref(audio_caps);
        return -1;
    }
    
    gst_caps_unref(video_caps);
    gst_caps_unref(audio_caps);
    
    // Connect to pad-added signal for dynamic pads from rtspsrc
    g_signal_connect(data.rtspsrc, "pad-added", G_CALLBACK(on_pad_added), &data);
    
    // Add bus watch
    bus = gst_pipeline_get_bus(GST_PIPELINE(data.pipeline));
    gst_bus_add_watch(bus, bus_callback, &data);
    gst_object_unref(bus);
    
    // Start playing
    g_print("Starting pipeline...\n");
    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
    
    // Create main loop
    data.loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(data.loop);
    
    // Cleanup
    g_print("Stopping pipeline...\n");
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);
    g_main_loop_unref(data.loop);
    
    return 0;
}
