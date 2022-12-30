// Include atomic std library
#include <atomic>
#include <iostream>
#include <mutex>
#include <thread>
#include <sstream>

// Include gstreamer library
#include <gst/gst.h>
#include <gst/app/app.h>
// Include OpenCV library
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>

const std::string kV4lDevice = "/dev/video0";
const std::string kDestAddress = "127.0.0.1";
constexpr auto kPort = 5000;

// Share frame between main loop and gstreamer callback
std::atomic<cv::Mat*> atomicFrame;

/**
 * @brief Check preroll to get a new frame using callback
 *  https://gstreamer.freedesktop.org/documentation/design/preroll.html
 * @return GstFlowReturn
 */
static GstFlowReturn new_preroll(GstAppSink* /*appsink*/, gpointer /*data*/)
{
    return GST_FLOW_OK;
}
/**
 * @brief This is a callback that get a new frame when a preroll exist
 *
 * @param appsink
 * @return GstFlowReturn
 */
static GstFlowReturn new_sample(GstAppSink *appsink, gpointer /*data*/)
{
    // Get caps and frame
    GstSample *sample = gst_app_sink_pull_sample(appsink);
    GstCaps *caps = gst_sample_get_caps(sample);
    GstBuffer *buffer = gst_sample_get_buffer(sample);
    GstStructure *structure = gst_caps_get_structure(caps, 0);
    const int width = g_value_get_int(gst_structure_get_value(structure, "width"));
    const int height = g_value_get_int(gst_structure_get_value(structure, "height"));

    // Get frame data
    GstMapInfo map;
    gst_buffer_map(buffer, &map, GST_MAP_READ);

    // Convert gstreamer data to OpenCV Mat
    cv::Mat* prevFrame;
    prevFrame = atomicFrame.exchange(new cv::Mat(cv::Size(width, height), CV_8UC3, (char*)map.data, cv::Mat::AUTO_STEP));
    if(prevFrame) {
        delete prevFrame;
    }
    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);
    return GST_FLOW_OK;
}

/**
 * @brief Bus callback
 *  Print important messages
 *
 * @param bus
 * @param message
 * @param data
 * @return gboolean
 */
static gboolean my_bus_callback(GstBus *bus, GstMessage *message, gpointer data)
{
    // Debug message
    switch(GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(message, &err, &debug);
            g_print("Error: %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            /* end-of-stream */
            break;
        default:
            /* unhandled message */
            break;
    }
    /* we want to be notified again the next time there is a message
     * on the bus, so returning TRUE (FALSE means we want to stop watching
     * for messages on the bus and our callback should not be called again)
     */
    return true;
}

void runPipeline(GstElement *pipeline, const std::string& src, cv::Mat & current_frame, std::mutex & mtx) {
    // Check pipeline
    GError *error = nullptr;
    pipeline = gst_parse_launch(src.c_str(), &error);
    if(error) {
        g_print("could not construct pipeline: %s\n", error->message);
        g_error_free(error);
        exit(-1);
    }
    // Get sink
    GstElement *sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
    /**
     * @brief Get sink signals and check for a preroll
     *  If preroll exists, we do have a new frame
     */
    gst_app_sink_set_emit_signals((GstAppSink*)sink, true);
    gst_app_sink_set_drop((GstAppSink*)sink, true);
    gst_app_sink_set_max_buffers((GstAppSink*)sink, 1);
    GstAppSinkCallbacks callbacks = { nullptr, new_preroll, new_sample };
    gst_app_sink_set_callbacks(GST_APP_SINK(sink), &callbacks, nullptr, nullptr);
    // Declare bus
    GstBus *bus;
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, my_bus_callback, nullptr);
    gst_object_unref(bus);
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
 
    // Create tracker, select region-of-interest (ROI) and initialize the tracker
    cv::Ptr<cv::Tracker> tracker = cv::TrackerKCF::create();

    bool flag = true;
    // Main loop
    int i = 0;
    cv::Rect box;
    cv::Rect2i rect;
    while(true) {
        g_main_context_iteration(g_main_context_default(), false);
        // std::lock_guard<std::mutex> lock(mtx);
        cv::Mat *frame = atomicFrame.load();
        if (frame) {
            current_frame = frame->clone();
            // cv::imshow("Frame", current_frame);
            // cv::waitKey(30);
        }
    }
    // gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);
    // gst_object_unref(GST_OBJECT(pipeline));
}

void stopPipeline(GstElement* pipeline) {
    gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED);
    // gst_object_unref(GST_OBJECT(pipeline));
}

class VideoStreamerMultisink {
    GstElement *_pipeline;
    std::string _pipeline_description;
    cv::Mat _current_frame;
    mutable std::mutex _mtx;
    std::thread t;
public:
    VideoStreamerMultisink(const std::string &destanationAddress, int port, const std::string &device) {
        std::stringstream pipelineString;

        pipelineString << "v4l2src name=src device=" << device << " ! videoconvert "
        "! tee name=t t. ! queue !  decodebin ! videoconvert ! video/x-raw ,format=(string)BGR ! videoconvert "
        "! appsink name=sink emit-signals=true sync=false max-buffers=1 drop=true "
        "t. ! queue ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert "
        "! x264enc tune=zerolatency ! rtph264pay ! udpsink host=" << destanationAddress << " port=" << std::to_string(port);
        _pipeline_description = pipelineString.str(); 
    }

    ~VideoStreamerMultisink() {
        gst_element_set_state(GST_ELEMENT(_pipeline), GST_STATE_NULL);
        gst_object_unref(GST_OBJECT(_pipeline));
        t.join();
    }

    void start() {
        t = std::thread([this] {
            runPipeline(_pipeline, _pipeline_description, _current_frame, _mtx);
        });
    }

    bool stop() {
        stopPipeline(_pipeline);
        return true;
    }

    // cv::Mat getCurrentFrame() const {
        // std::lock_guard<std::mutex> lock(_mtx);
        // return _current_frame;
    // }
};

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    // std::string descr =
    //     "v4l2src name=src device=" + kV4lDevice + " ! videoconvert "
    //     "! tee name=t t. ! queue !  decodebin ! videoconvert ! video/x-raw ,format=(string)BGR ! videoconvert "
    //     "! appsink name=sink emit-signals=true sync=false max-buffers=1 drop=true "
    //     "t. ! queue ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert "
    //     "! x264enc tune=zerolatency ! rtph264pay ! udpsink host=" + kDestAddress +" port=" + std::to_string(kPort)
    // ;

    // runPipeline(descr);
    VideoStreamerMultisink streamer("127.0.0.1", 5000, "/dev/video0");
    streamer.start();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    streamer.stop();
    return 0;
}
