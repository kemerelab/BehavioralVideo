#ifndef FFMPEG_H
#define FFMPEG_H

//namespace ffmpeg{
extern "C"{
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavcodec/avcodec.h"
#include "libavutil/opt.h"
}
//}

Q_DECLARE_METATYPE(AVCodecID)

#endif // FFMPEG_H
