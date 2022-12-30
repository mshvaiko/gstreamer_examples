#include <iostream>
#include <opencv4/opencv2/highgui/highgui.hpp>
#include <opencv4/opencv2/opencv.hpp>

int main()
{
    cv::VideoCapture cap("libcamerasrc \
                        ! video/x-raw, width=640,height=480,framerate=30/1 \
                        ! videoconvert \
                        ! appsink", cv::CAP_GSTREAMER);
    cv::VideoWriter out("appsrc \
                        ! videoconvert \
                        ! videoconvert ! videoscale ! clockoverlay time-format=\"%D %H:%M:%S\" \
                        ! x265enc tune=zerolatency bitrate=500 speed-preset=superfast bitrate=800 \
                        ! rtph265pay \
                        ! udpsink host=127.0.0.1 port=5000",
                        cv::CAP_GSTREAMER, 0, 30, cv::Size(640, 480), true);

    if (!cap.isOpened() || !out.isOpened())
    {
        std::cout << "VideoCapture or VideoWriter not opened" << std::endl;
        exit(-1);
    }

    cv::Mat frame;

    while (true) {
        cap.read(frame);

        if (frame.empty())
            break;

        out.write(frame);

        cv::imshow("Sender", frame);
        if (cv::waitKey(1) == 's')
            break;
    }
    cv::destroyWindow("Sender");
    return 0;
}