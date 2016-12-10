CFLAGS = -Wall -g `pkg-config --cflags gstreamer-1.0`
LDFLAGS = `pkg-config --libs gstreamer-1.0`

all:	rtp_client.c 


clean:
        rm -rf *.o *~ 
