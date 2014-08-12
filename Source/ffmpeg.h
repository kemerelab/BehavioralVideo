#ifndef FFMPEG_H
#define FFMPEG_H

//namespace ffmpeg{
extern "C"{
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
}
//}

#endif // FFMPEG_H
