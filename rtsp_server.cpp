#include <gst/gst.h>
#include <gst/rtsp-server/rtsp-server.h>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    // Check if filename is provided
    if (argc != 2) {
        g_printerr("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    std::string filename = argv[1];
    gst_init(&argc, &argv);

    GstRTSPServer *server = gst_rtsp_server_new();
    gst_rtsp_server_set_service(server, "8554");

    GstRTSPMountPoints *mounts = gst_rtsp_server_get_mount_points(server);
    GstRTSPMediaFactory *factory = gst_rtsp_media_factory_new();

    // Create the pipeline string with the provided filename
    std::string pipeline = "( filesrc location=" + filename + " ! " +
                         "decodebin name=decoder "
                         "decoder. ! queue ! videoconvert ! videoscale ! video/x-raw,width=640,height=480 ! "
                         "vp8enc deadline=1 ! rtpvp8pay name=pay0 pt=96 "
                         "decoder. ! queue ! audioconvert ! audioresample ! opusenc ! rtpopuspay name=pay1 pt=97 )";

    gst_rtsp_media_factory_set_launch(factory, pipeline.c_str());
    gst_rtsp_media_factory_set_shared(factory, TRUE);

    gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
    g_object_unref(mounts);

    gst_rtsp_server_attach(server, NULL);

    g_print("RTSP server running at rtsp://127.0.0.1:8554/test\n");
    g_print("Streaming file: %s\n", filename.c_str());

    GMainLoop *loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(loop);

    return 0;
}