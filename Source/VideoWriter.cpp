#include "VideoWriter.h"
#include <QDebug>
#include <QThread>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>

VideoWriter::VideoWriter(QObject *parent) :
    QObject(parent)
{
    width = -1;
    height = -1;
    timeBase = (AVRational){1,30};
    waitingToInitialize = false;
    currentlyWriting = false;
    frameTimer = NULL;
}

void VideoWriter::addPreferencesPanel(QTabWidget *preferencesTabs)
{
    prefWidget = new QWidget(preferencesTabs->parentWidget());

    QLineEdit *defaultExtension = new QLineEdit(videoExt,prefWidget);
    videoEncoder = new QComboBox(prefWidget);
    videoEncoder->addItem("MPEG1",QVariant((unsigned int) AV_CODEC_ID_MPEG1VIDEO));
    videoEncoder->addItem("MPEG2",QVariant((unsigned int) AV_CODEC_ID_MPEG2VIDEO));
    videoEncoder->addItem("H264",QVariant((unsigned int) AV_CODEC_ID_H264));
    videoEncoder->setCurrentIndex(1); // MPEG2

    QFormLayout *layout = new QFormLayout(prefWidget);
    layout->addRow("Default File Extension", defaultExtension);
    layout->addRow("Video Encoder", videoEncoder);
    preferencesTabs->addTab(prefWidget,"Video Writer");

    // safe now because we've moved to our own thread.
    if (!frameTimer)
        frameTimer = new QElapsedTimer();


}

void VideoWriter::initialize(QString filename)
{

    filename = filename + QString(".mp4");
    qDebug() << "Initializing writing to " << filename;

    vFilename = &filename;
    if ((width == -1) || (height == -1)) {
        // Haven't received any frames yet, so we don't know what size they are...
        waitingToInitialize = true;
    }
    else {
        //qDebug() << "Thread for initialize: " << QThread::currentThreadId();
        // Write header here
        // FFMPEG initialization
        av_register_all(); // initialize libavcodec, and register all codecs and formats
        fmt = av_guess_format("mp4", NULL, NULL);
        if (!fmt) {
            qCritical() << "Error initializing fmt.";
        }
        fmt->video_codec = (AVCodecID) videoEncoder->currentData().value<unsigned int>();

        /* allocate the output media context */
        oc = avformat_alloc_context();
        if (!oc)
            qCritical() << "Error allocating output media context.";

        qDebug() << "Output context allocated";

        oc->oformat = fmt;
        qsnprintf(oc->filename, sizeof(oc->filename), "%s", vFilename->toLocal8Bit().data());

        /* add the video stream to the container and initialize the codecs */
        video_st = NULL;

        AVCodec *codec = avcodec_find_encoder(fmt->video_codec); // should add error checking
        video_st = avformat_new_stream(oc, codec); // should add error checking

        AVCodecContext *c = video_st->codec;
        c->codec_id = fmt->video_codec;
        c->codec_type = AVMEDIA_TYPE_VIDEO;

        /* put sample parameters */
        c->width = width;
        c->height = height; // resolution must be a multiple of two
        c->pix_fmt = PIX_FMT_YUV420P;
        c->time_base = timeBase; // frames per second

        // Stuff to try to force high quality encoding
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            c->qmax = 1; // low is better, so frame-by-frame force high quality
            /* just for testing, we also get rid of B frames */
            //c->max_b_frames = 0;
            //c->gop_size = 1; /* emit one intra frame every n frames at most */
            //c->bit_rate = 6000000;
        }
        else if (c->codec_id == AV_CODEC_ID_H264) {
            av_opt_set(c->priv_data, "preset", "ultrafast",0);
            //c->qmax = 18;
            //c->qmin = 18;
        }
        // some formats want stream headers to be seperate
        if (oc->oformat->flags & AVFMT_GLOBALHEADER)
            c->flags |= CODEC_FLAG_GLOBAL_HEADER;

        //av_dump_format(oc, 0, vFilename->toLocal8Bit().data(), 1);

        /* now that all the parameters are set, we can open the
           video codec and allocate the necessary encode buffers */
        /* open the codec */
        int err = avcodec_open2(c, codec, NULL); // should error check

        video_outbuf = NULL;
        if (!(oc->oformat->flags & AVFMT_RAWPICTURE)) {
            /* allocate output buffer */
            video_outbuf_size = 1000000;
            video_outbuf = (uint8_t*)malloc(video_outbuf_size);
        }

        picture = avcodec_alloc_frame(); // should error check
        if (!picture) {
            qCritical() << "Error allocating picture frame.";
        }
        int size = avpicture_get_size(c->pix_fmt, width, height);
        picture_buf = (uint8_t*)av_malloc(size); // should error check
        if (picture_buf == NULL) {
            qCritical() << "Error allocating memory for picture buffer.";
        }
        avpicture_fill((AVPicture *)picture, picture_buf, c->pix_fmt, width, height);

        /* open the output file, if needed */
        if (avio_open(&oc->pb, vFilename->toLocal8Bit().data(), AVIO_FLAG_WRITE) < 0) {
            qDebug() << "Could not open " << vFilename;
        }

        /* write the stream header, if any */
        avformat_write_header(oc, NULL);

        // Open subtitles here

        // Set up frame conversion from RGB24 to YUV420P
        sws_ctx = sws_getContext(c->width, c->height, PIX_FMT_RGB24,
                                 c->width, c->height, c->pix_fmt,
                                 SWS_FAST_BILINEAR | SWS_CPU_CAPS_SSE2 | SWS_CPU_CAPS_MMX2,
                                 NULL, NULL, NULL);
        if (!sws_ctx) {
            qCritical() << "Error allocating SWS context.";
        }
        tmp_picture = avcodec_alloc_frame();
        if (!tmp_picture) {
            qCritical() << "Error allocating tmp_picture frame.";
        }

        waitingToInitialize = false;
        qDebug() << "Video Initialized emitted";

        // Should check here for errors above
    }


}

void VideoWriter::newFrame(QVideoFrame frame)
{
    if ((frame.width() != width) || (frame.height() != height)) {
        width = frame.width();
        height = frame.height();
    }
    if (waitingToInitialize) {
        initialize(*vFilename);
    }
    if (currentlyWriting) {
        if (!frame.map(QAbstractVideoBuffer::ReadOnly)) {
            qDebug() << "Failure to map video frame in writer";
            return;
        }

        AVCodecContext *c = video_st->codec;
        avpicture_fill((AVPicture *)tmp_picture, frame.bits(),
                       PIX_FMT_RGB24, c->width, c->height);
        sws_scale(sws_ctx, tmp_picture->data, tmp_picture->linesize,
                  0, c->height, picture->data, picture->linesize);
        picture->pts = frameCounter++;
        frame.unmap();

        /* encode the image */
        /* if zero size, it means the image was buffered */
        /* write the compressed frame in the media file */
        /* XXX: in case of B frames, the pts is not yet valid */
        int out_size = avcodec_encode_video(c, video_outbuf, video_outbuf_size, picture);
        if (out_size > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (c->coded_frame->pts != AV_NOPTS_VALUE)
                pkt.pts= av_rescale_q(c->coded_frame->pts, c->time_base, video_st->time_base);
            if(c->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index= video_st->index;
            pkt.data= video_outbuf;
            pkt.size= out_size;

            /* write the compressed frame in the media file */
            //ret = av_interleaved_write_frame(oc, &pkt);
            int ret = av_write_frame(oc, &pkt);
        }


        // Save time stamp
    }
}

void VideoWriter::beginWriting()
{
    qDebug() << "begin writing";
    currentlyWriting = true;
    frameCounter = 0;
}

void VideoWriter::endWriting()
{
    qDebug() << "End writing";
    if (currentlyWriting) {
        currentlyWriting = false;
        int out_size;
        while ((out_size = avcodec_encode_video(video_st->codec, video_outbuf, video_outbuf_size, NULL)) > 0) {
            AVPacket pkt;
            av_init_packet(&pkt);
            if (video_st->codec->coded_frame->pts != AV_NOPTS_VALUE)
                pkt.pts= av_rescale_q(video_st->codec->coded_frame->pts,
                                      video_st->codec->time_base, video_st->time_base);
            if(video_st->codec->coded_frame->key_frame)
                pkt.flags |= AV_PKT_FLAG_KEY;
            pkt.stream_index= video_st->index;
            pkt.data= video_outbuf;
            pkt.size= out_size;
            int ret = av_write_frame(oc, &pkt);
        }

        // write FFMPEG trailer
        av_write_trailer(oc);

        /* close each codec */
        if (video_st) {
            avcodec_close(video_st->codec);
            av_free(picture->data[0]);
            av_free(picture);

            av_free(video_outbuf);
            video_outbuf=NULL;

            av_free(tmp_picture);
            tmp_picture=NULL;
        }

        avio_close(oc->pb);
        /* free the stream and context */
        avformat_free_context(oc);
        oc=NULL;

        // close(info->fdts);
    }

}
