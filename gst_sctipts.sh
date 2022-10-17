#stream webcam
gst-launch-1.0 -v v4l2src device=/dev/video0 ! "image/jpeg,width=1280, height=720,framerate=30/1" ! rtpjpegpay ! udpsink host=127.0.0.1 port=5000

#receive stream
gst-launch-1.0 -e -v udpsrc port=5000 ! application/x-rtp, encoding-name=JPEG, payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink

#below script line is working on RPi3 and webcam setup to stream video throw the network
gst-launch-1.0 -v v4l2src device=/dev/video0 num-buffers=-1 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! jpegenc ! rtpjpegpay ! udpsink host=192.168.0.174 port=5000

#below script line is tested on Rpi3 and webcam setup to receive stream throw the network
gst-launch-1.0 -e -v udpsrc port=5000 ! application/x-rtp, encoding-name=JPEG, payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink

# good guideline link https://qengineering.eu/install-gstreamer-1.18-on-raspberry-pi-4.html

# stream wecam works with qgroundcontroll
gst-launch-1.0 -v v4l2src device=/dev/video0 ! video/x-raw, width=640, height=480, framerate=30/1 ! videoconvert ! x264enc tune=zerolatency ! rtph264pay ! udpsink host=127.0.0.1 port=5000
