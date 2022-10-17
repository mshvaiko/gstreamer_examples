## Receiving part
>gst-launch-1.0 -vvv udpsrc port=5000 caps="application/x-rtp" ! rtph264depay ! avdec_h264 ! videoconvert ! xvimagesink

## Updated receive command after code update (using webcam as video source and jpeg as encoder)
>gst-launch-1.0 -e -v udpsrc port=5000 ! application/x-rtp, encoding-name=JPEG, payload=26 ! rtpjpegdepay ! jpegdec ! autovideosink
