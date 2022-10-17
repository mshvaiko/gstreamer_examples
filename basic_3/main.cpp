#include <iostream>
#include <chrono>
#include <thread>

#include <gst/gst.h>

constexpr auto kHost = "127.0.0.1";
constexpr auto kPort = 5000;
constexpr auto kV4lDevice = "/dev/video0";

class VideoStreamer
{
private:
    GMainLoop *loop = nullptr;
    GstElement *pipeline = nullptr;
    std::thread pipeline_thread;

public:
    VideoStreamer(const std::string &destanationAddress, int port, const std::string &device = kV4lDevice)
        : loop(g_main_loop_new(nullptr, false)), pipeline(gst_pipeline_new(device.c_str()))
    {
        auto videosrc = gst_element_factory_make("v4l2src", "source");
        auto converter = gst_element_factory_make("videoconvert", "conv");
        auto encoder = gst_element_factory_make("jpegenc", "enc");
        auto payload = gst_element_factory_make("rtpjpegpay", "pay");

        g_object_set(G_OBJECT(videosrc), "device", kV4lDevice, nullptr);

        auto udp = gst_element_factory_make("udpsink", "udp");
        g_object_set(G_OBJECT(udp), "host", destanationAddress.c_str(), nullptr);
        g_object_set(G_OBJECT(udp), "port", kPort, nullptr);

        gst_bin_add_many(GST_BIN(pipeline), videosrc, converter, encoder, payload, udp, nullptr);

        if (gst_element_link_many(videosrc, converter, encoder, payload, udp, nullptr) != true)
        {
            std::cout << "Failed to link gst elements..." << std::endl;
            // TODO: throw some error or move initialization outside from constructor
        }
        pipeline_thread = std::thread([this] {
            g_main_loop_run(loop);
        });
    }

    bool start()
    {
        return GST_STATE_CHANGE_FAILURE != gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }

    bool stop()
    {
        return GST_STATE_CHANGE_FAILURE != gst_element_set_state(pipeline, GST_STATE_PAUSED);
    }

    ~VideoStreamer()
    {
        if (pipeline_thread.joinable()) {
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

    VideoStreamer streamer(kHost, kPort, kV4lDevice);

    if (streamer.start()) {
        std::cout << "Good" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    if (streamer.stop()) {
        std::cout << "Good" << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    streamer.start();

    return 0;
}