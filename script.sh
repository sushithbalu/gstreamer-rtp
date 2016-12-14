final:

gst-launch-1.0 -v  v4l2src device=/dev/video0 ! videoconvert ! queue !  x264enc tune=zerolatency speed-preset=superfast ! queue ! mux2. pulsesrc !   audioresample ! audioconvert ! queue ! faac ! queue ! mux2. mpegtsmux name="mux2" ! queue ! rtpmp2tpay ! udpsink host=localhost port=5000 sync=true

gst-launch-1.0 udpsrc port=5000 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)MP2T, payload=(int)33, seqnum-offset=(uint)21229, timestamp-offset=(uint)4161967118, ssrc=(uint)1854310367" ! queue ! rtpmp2tdepay ! queue ! tsparse ! demux. tsdemux name=demux ! video/x-h264,stream-format=byte-stream,profile=high ! h264parse ! avdec_h264 ! queue ! videoconvert ! autovideosink demux. ! queue ! aacparse ! faad ! queue ! audioconvert ! autoaudiosink
gst-launch-1.0 -v udpsrc port=5000 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)MP2T, payload=(int)33, seqnum-offset=(uint)17490, timestamp-offset=(uint)2217187636, ssrc=(uint)4010651969" ! queue ! rtpbin ! rtpmp2tdepay ! queue ! tsparse ! demux. tsdemux name=demux ! queue ! h264parse ! avdec_h264 ! queue ! videoconvert ! autovideosink demux. ! queue ! aacparse ! faad ! queue ! audioconvert ! autoaudiosink
-----------------------------------------------------------------------------------------------

gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! queue ! x264enc \
! mux2. pulsesrc ! queue ! audioconvert  !  \
mux2. mpegtsmux name="mux2" ! rtpmp2tpay ! udpsink host=localhost port=5000 sync=true


gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! queue !
 x264enc pass=pass1 threads=0 bitrate=900000 tune=zerolatency !
 mux2. pulsesrc ! queue ! audioconvert ! faac  ! mux2. mpegtsmux name="mux2" !
 rtpmp2tpay ! udpsink host=localhost port=5000 sync=true


gst-launch-1.0 -v v4l2src device=/dev/video0 ! videoconvert ! queue !
 x264enc pass=pass1 threads=0 bitrate=900000 tune=zerolatency ! queue !
 mux2. pulsesrc ! audioconvert ! queue ! faac ! mux2. mpegtsmux name="mux2" ! queue !
 rtpmp2tpay ! udpsink host=localhost port=5000 sync=true

gst-launch-1.0 -v  v4l2src device=/dev/video0 ! videoconvert ! queue !
 x264enc tune=zerolatency speed-preset=superfast ! queue ! mux2. pulsesrc ! 
 audioresample ! audioconvert ! queue ! faac ! queue ! mux2. mpegtsmux name="mux2" !
 rtpmp2tpay ! udpsink host=192.168.1.101 port=5000 sync=false



gst-launch-1.0 -v udpsrc port=5000 caps = "application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)MP2T, payload=(int)33, seqnum-offset=(uint)26325, timestamp-offset=(uint)2509556693, ssrc=(uint)2428422961" ! queue ! rtpbin ! rtpmp2tdepay ! demux. tsdemux name=demux ! queue ! h264parse ! avdec_h264 ! queue ! videoconvert ! videoscale ! autovideosink demux. ! queue ! aacparse ! faad ! audioconvert ! autoaudiosink --gst-debug-level=3

gst-launch-1.0 -vvvv  v4l2src device=/dev/video0 num-buffers=150 ! videoconvert ! queue !  x264enc tune=zerolatency speed-preset=superfast ! queue ! mux2. pulsesrc !   audioresample ! audioconvert ! queue ! faac ! queue ! mux2. mpegtsmux name="mux2" ! queue ! rtpmp2tpay ! udpsink host=localhost port=5000 sync=false
