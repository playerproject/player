/* $Id$
 *
 * The base class for capture classes that feed data to CMVision
 */

#ifndef __CAPTURE_H__
#define __CAPTURE_H__

typedef long long stamp_t;

class capture 
{
  protected:
    unsigned char *current; // most recently captured frame
    stamp_t timestamp;      // frame time stamp
    int width,height;       // dimensions of video frame
    bool captured_frame;

  public:
    capture() {current=NULL; captured_frame = false;}

    // you must define these in the subclass
    virtual bool initialize(int nwidth,int nheight) = 0;
    virtual void close() = 0;
    virtual unsigned char *captureFrame() = 0;
    
    // are these needed?
    /*
    bool initialize(char *device,int nwidth,int nheight,int nfmt)
      { return initialize(nwidth,nheight); }
    bool initialize() { return initialize(0,0); }
    unsigned char *captureFrame(int &index,int &field);
    void releaseFrame(unsigned char* frame, int index);
    unsigned char *getFrame() {return(current);}
    stamp_t getFrameTime() {return(timestamp);}
    double getFrameTimeSec() {return(timestamp * 1.0E-9);}
    int getWidth() {return(width);}
    int getHeight() {return(height);}
    */
};

#endif // __CAPTURE_H__
