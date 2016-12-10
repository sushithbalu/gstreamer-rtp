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
