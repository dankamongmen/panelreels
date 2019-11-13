#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

int load_image_into_frame(AVFrame *frame, const char *filename){
  int retval = -1, res;
  static struct SwsContext *sws_ctx;
  uint8_t *image_data[4];
  int linesize[4];
  int source_width, source_height;
  enum AVPixelFormat source_fmt;

  res = ff_load_image(image_data, linesize, &source_width, &source_height, &source_fmt, filename, NULL);
  if(res < 0){
    fprintf(stderr, "Error loading image at %s", filename);
    return -1;
  }

  if (source_fmt != frame->format) {
    sws_ctx = sws_getContext(source_width, source_height, source_fmt,
        frame->width, frame->height, frame->format,
        0, NULL, NULL, NULL);
    if(sws_ctx == NULL){
      fprintf(stderr, "Error initializing scaling context");
      goto error;
    }

    sws_scale(sws_ctx,
        (const uint8_t * const *)image_data, linesize,
        0, frame->height, frame->data, frame->linesize);
  }
  retval = 0;

error:
  av_freep(&image_data[0]);
  sws_freeContext(sws_ctx);
  return retval;
}
