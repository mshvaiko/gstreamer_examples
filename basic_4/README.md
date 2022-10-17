## Receiving part
>gst-launch-1.0 -vvv udpsrc port=5000 caps="application/x-rtp" ! rtph264depay ! avdec_h264 ! videoconvert ! xvimagesink
