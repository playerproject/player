#ifndef _V4L2_H
#define _V4L2_H

#ifdef __cplusplus
extern "C" {
#endif

extern void * open_fg(const char * dev, const char * pixformat, int width, int height, int imgdepth, int buffers);
extern void close_fg(void * fg);
extern int set_channel(void * fg, int channel, const char * mode);
extern int start_grab (void * fg);
extern void stop_grab (void * fg);
extern unsigned char * get_image(void * fg);
extern int fg_width(void * fg);
extern int fg_height(void * fg);
extern int fg_grabdepth(void * fg);
extern int fg_imgdepth(void * fg);

#ifdef __cplusplus
}
#endif

#endif
