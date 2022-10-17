#include <iostream>
#include <chrono>
#include <thread>

#include <gst/gst.h>

constexpr auto kHost = "mshvaiko.local";
constexpr auto kPort = 5000;
constexpr auto kV4lDevice0 = "/dev/video0";
constexpr auto kV4lDevice2 = "/dev/video2";

class VideoStreamer
{
private:
    GMainLoop *loop = nullptr;
    GstElement *pipeline = nullptr;
    std::thread pipeline_thread;

public:
    VideoStreamer(const std::string &destanationAddress, int port, const std::string &device = kV4lDevice0)
        : loop(g_main_loop_new(nullptr, false)), pipeline(gst_pipeline_new(device.c_str()))
    {
        // auto videosrc = gst_element_factory_make("v4l2src", "source");
        auto videosrc = gst_element_factory_make("videotestsrc", "source");
        auto converter = gst_element_factory_make("videoconvert", "conv");
        auto encoder = gst_element_factory_make("x264enc", "enc");
        auto payload = gst_element_factory_make("rtph264pay", "pay");
        auto udp = gst_element_factory_make("udpsink", "udp");

        // g_object_set(G_OBJECT(videosrc), "device", device.c_str(), NULL);
        g_object_set(G_OBJECT(videosrc), "pattern", 24, NULL);
        g_object_set(G_OBJECT(udp), "host", destanationAddress.c_str(), nullptr);
        g_object_set(G_OBJECT(udp), "port", port, nullptr);
        g_object_set(encoder, "tune", 4, NULL); // zerolatency

        if (!videosrc || !converter || !encoder || !payload)
        {
            std::cout << "Fuck..." << std::endl;
        }
        gst_bin_add_many(GST_BIN(pipeline), videosrc, converter, encoder, payload, udp, nullptr);

        if (gst_element_link_many(videosrc, converter, encoder, payload, udp, nullptr) != true)
        {
            // TODO: error handling
            std::cout << "Failed to link gst elements..." << std::endl;
        }

        pipeline_thread = std::thread([this]
                                      { g_main_loop_run(loop); });
    }

    bool start()
    {
        std::cout << "Stream: " << pipeline->object.name << " has been started" << std::endl;
        return GST_STATE_CHANGE_FAILURE != gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }

    bool isPlaying() {
        GstState state;
        gst_element_get_state(pipeline, &state, NULL, GST_CLOCK_TIME_NONE);
        return state == GST_STATE_PLAYING;
    }

    bool stop()
    {
        return GST_STATE_CHANGE_FAILURE != gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }

    ~VideoStreamer()
    {
        if (pipeline_thread.joinable())
        {
            pipeline_thread.join();
        }

        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(pipeline));
        g_main_loop_unref(loop);
    }
};

int main(int argc, char *argv[])
{
    gst_init(nullptr, nullptr);

    VideoStreamer streamer1(kHost, kPort, kV4lDevice0);
    VideoStreamer streamer2(kHost, kPort + 1, kV4lDevice2);

    if (streamer1.start())
    {
        std::cout << "Good streamer1 started on port: " << kPort << std::endl;
    }

    std::cout << "isPlaying: " << streamer1.isPlaying() << std::endl;

    if (streamer2.start())
    {
        std::cout << "Good streamer2 started on port: " << kPort + 1 << std::endl;
    }

    return 0;
}
