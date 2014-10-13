/* qv4l2: a control panel controlling v4l2 devices.
 *
 * Copyright (C) 2006 Hans Verkuil <hverkuil@xs4all.nl>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

//#include "qv4l2.h"
#include "general-tab.h"
#include "../SupportedCamera.h"

#include <QToolBar>
#include <QToolButton>
#include <QMenuBar>
#include <QFileDialog>
#include <QStatusBar>
#include <QApplication>
#include <QMessageBox>
#include <QLineEdit>
#include <QValidator>
#include <QLayout>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QToolTip>
#include <QImage>
#include <QWhatsThis>
#include <QThread>
#include <QCloseEvent>
#include <QDebug>

#include <assert.h>
#include <sys/mman.h>
#include <errno.h>
#include <dirent.h>
#include <libv4l2.h>


void SupportedCamera::setDevice(const QString &device, bool rawOpen)
{
	closeDevice();
	m_sigMapper = new QSignalMapper(this);
	connect(m_sigMapper, SIGNAL(mapped(int)), this, SLOT(ctrlAction(int)));

	if (!open(device, !rawOpen))
		return;

    QWidget *w = new QWidget(prefPanel->parentWidget());
    prefPanel->addTab(w,"V4L2 " + device);
    m_mainLayout = new QGridLayout(w);
    QWidget *t = new QWidget(w);
    m_mainLayout->addWidget(t,0,0);
    m_genTab = new GeneralTab(device, *this, 4, t);
    m_tabs = new QWidget(w);
    m_mainLayout->addWidget(m_tabs,1,0);
    addControlWidgets();
    m_convertData = v4lconvert_create(fd());

}

//void SupportedCamera::initializeConversion(void)
//{
//}



void SupportedCamera::capFrame()
{
	__u32 buftype = m_genTab->bufType();
	v4l2_buffer buf;
	int s = 0;
	int err = 0;
	bool again;

	switch (m_capMethod) {
	case methodRead:
		s = read(m_frameData, m_capSrcFormat.fmt.pix.sizeimage);
		if (s < 0) {
			if (errno != EAGAIN) {
				error("read");
			}
			return;
		}

        if (!m_capImage->map(QAbstractVideoBuffer::WriteOnly)) {
            qDebug() << "Failure to map frame in capture (read).";
        }
        else {
            if (m_mustConvert)
                err = v4lconvert_convert(m_convertData, &m_capSrcFormat, &m_capDestFormat,
                                         m_frameData, s,
                                         m_capImage->bits(), m_capDestFormat.fmt.pix.sizeimage);
            if (!m_mustConvert || err < 0)
                memcpy(m_capImage->bits(), m_frameData, std::min(s, m_capImage->mappedBytes()));

            m_capImage->unmap();
        }
        break;

	case methodMmap:
		if (!dqbuf_mmap(buf, buftype, again)) {
			error("dqbuf");
			return;
		}
		if (again)
			return;

        if (!m_capImage->map(QAbstractVideoBuffer::WriteOnly)) {
            qDebug() << "Failure to map frame in capture (mmap).";
        }
        else {
            if (m_mustConvert)
                err = v4lconvert_convert(m_convertData,
                                         &m_capSrcFormat, &m_capDestFormat,
                                         (unsigned char *)m_buffers[buf.index].start, buf.bytesused,
                        m_capImage->bits(), m_capDestFormat.fmt.pix.sizeimage);
            if (!m_mustConvert || err < 0)
                memcpy(m_capImage->bits(),
                       (unsigned char *)m_buffers[buf.index].start,
                        std::min(buf.bytesused, (unsigned)m_capImage->mappedBytes()));
            m_capImage->unmap();
            qbuf(buf);
        }
		break;

	case methodUser:
		if (!dqbuf_user(buf, buftype, again)) {
			error("dqbuf");
			return;
		}
		if (again)
			return;

        if (!m_capImage->map(QAbstractVideoBuffer::WriteOnly)) {
            qDebug() << "Failure to map frame in capture (user).";
        }
        else {
            if (m_mustConvert)
                err = v4lconvert_convert(m_convertData,
                                         &m_capSrcFormat, &m_capDestFormat,
                                         (unsigned char *)buf.m.userptr, buf.bytesused,
                                         m_capImage->bits(), m_capDestFormat.fmt.pix.sizeimage);
            if (!m_mustConvert || err < 0)
                memcpy(m_capImage->bits(), (unsigned char *)buf.m.userptr,
                       std::min(buf.bytesused, (unsigned)m_capImage->mappedBytes()));

            m_capImage->unmap();
            qbuf(buf);
        }
		break;
	}
    if (err == -1 && m_frame++ == 0)
		error(v4lconvert_get_error_message(m_convertData));

    if (m_frame == 1)
        refresh();

    emit newFrame(*m_capImage);

}

bool SupportedCamera::initiateCapture(unsigned buffer_size)
{
	__u32 buftype = m_genTab->bufType();
	v4l2_requestbuffers req;
	unsigned int i;

	memset(&req, 0, sizeof(req));

	switch (m_capMethod) {
	case methodRead:
		/* Nothing to do. */
		return true;

	case methodMmap:
		if (!reqbufs_mmap(req, buftype, 3)) {
			error("Cannot capture");
			break;
		}

		if (req.count < 2) {
			error("Too few buffers");
			reqbufs_mmap(req, buftype);
			break;
		}

		m_buffers = (buffer *)calloc(req.count, sizeof(*m_buffers));

		if (!m_buffers) {
			error("Out of memory");
			reqbufs_mmap(req, buftype);
			break;
		}

		for (m_nbuffers = 0; m_nbuffers < req.count; ++m_nbuffers) {
			v4l2_buffer buf;

			memset(&buf, 0, sizeof(buf));

			buf.type        = buftype;
			buf.memory      = V4L2_MEMORY_MMAP;
			buf.index       = m_nbuffers;

			if (-1 == ioctl(VIDIOC_QUERYBUF, &buf)) {
				perror("VIDIOC_QUERYBUF");
				goto error;
			}

			m_buffers[m_nbuffers].length = buf.length;
			m_buffers[m_nbuffers].start = mmap(buf.length, buf.m.offset);

			if (MAP_FAILED == m_buffers[m_nbuffers].start) {
				perror("mmap");
				goto error;
			}
		}
		for (i = 0; i < m_nbuffers; ++i) {
			if (!qbuf_mmap(i, buftype)) {
				perror("VIDIOC_QBUF");
				goto error;
			}
		}
		if (!streamon(buftype)) {
			perror("VIDIOC_STREAMON");
			goto error;
		}
		return true;

	case methodUser:
		if (!reqbufs_user(req, buftype, 3)) {
			error("Cannot capture");
			break;
		}

		if (req.count < 2) {
			error("Too few buffers");
			reqbufs_user(req, buftype);
			break;
		}

		m_buffers = (buffer *)calloc(req.count, sizeof(*m_buffers));

		if (!m_buffers) {
			error("Out of memory");
			break;
		}

		for (m_nbuffers = 0; m_nbuffers < req.count; ++m_nbuffers) {
			m_buffers[m_nbuffers].length = buffer_size;
			m_buffers[m_nbuffers].start = malloc(buffer_size);

			if (!m_buffers[m_nbuffers].start) {
				error("Out of memory");
				goto error;
			}
		}
		for (i = 0; i < m_nbuffers; ++i)
			if (!qbuf_user(i, buftype, m_buffers[i].start, m_buffers[i].length)) {
				perror("VIDIOC_QBUF");
				goto error;
			}
		if (!streamon(buftype)) {
			perror("VIDIOC_STREAMON");
			goto error;
		}
		return true;
	}

error:
	return false;
}

void SupportedCamera::haltCapture()
{
	__u32 buftype = m_genTab->bufType();
	v4l2_requestbuffers reqbufs;
	v4l2_encoder_cmd cmd;
	unsigned i;

	switch (m_capMethod) {
	case methodRead:
		memset(&cmd, 0, sizeof(cmd));
		cmd.cmd = V4L2_ENC_CMD_STOP;
		ioctl(VIDIOC_ENCODER_CMD, &cmd);
		break;

	case methodMmap:
		if (m_buffers == NULL)
			break;
		if (!streamoff(buftype))
			perror("VIDIOC_STREAMOFF");
		for (i = 0; i < m_nbuffers; ++i)
			if (-1 == munmap(m_buffers[i].start, m_buffers[i].length))
				perror("munmap");
		// Free all buffers.
		reqbufs_mmap(reqbufs, buftype, 1);  // videobuf workaround
		reqbufs_mmap(reqbufs, buftype, 0);
		break;

	case methodUser:
		if (!streamoff(buftype))
			perror("VIDIOC_STREAMOFF");
		// Free all buffers.
		reqbufs_user(reqbufs, buftype, 1);  // videobuf workaround
		reqbufs_user(reqbufs, buftype, 0);
		for (i = 0; i < m_nbuffers; ++i)
			free(m_buffers[i].start);
		break;
	}
	free(m_buffers);
	m_buffers = NULL;
	refresh();
}

void SupportedCamera::capStart(bool start)
{
	QImage::Format dstFmt = QImage::Format_RGB888;
	struct v4l2_fract interval;
	v4l2_pix_format &srcPix = m_capSrcFormat.fmt.pix;
	v4l2_pix_format &dstPix = m_capDestFormat.fmt.pix;

	if (!start) {
        haltCapture();
		delete m_capNotifier;
		m_capNotifier = NULL;
		delete m_capImage;
		m_capImage = NULL;
		return;
	}
	m_frame = m_lastFrame = m_fps = 0;
    m_capMethod = m_genTab->capMethod();

	g_fmt_cap(m_capSrcFormat);
	s_fmt(m_capSrcFormat);
	if (m_genTab->get_interval(interval))
		set_interval(interval);

    m_mustConvert = true;
	m_frameData = new unsigned char[srcPix.sizeimage];
    m_capDestFormat = m_capSrcFormat;
    dstPix.pixelformat = V4L2_PIX_FMT_RGB24;

    if (srcPix.pixelformat == V4L2_PIX_FMT_RGB24)
        m_mustConvert = false;

    if (m_mustConvert) {
        v4l2_format copy = m_capSrcFormat;

        v4lconvert_try_format(m_convertData, &m_capDestFormat, &m_capSrcFormat);
        // v4lconvert_try_format sometimes modifies the source format if it thinks
        // that there is a better format available. Restore our selected source
        // format since we do not want that happening.
        m_capSrcFormat = copy;
        m_capImage = new QVideoFrame(dstPix.sizeimage, QSize(dstPix.width,dstPix.height),
                                     dstPix.bytesperline, QVideoFrame::Format_RGB24);
        //m_capImage->fill(0);
    }

    if (initiateCapture(srcPix.sizeimage)) {
        m_capNotifier = new QSocketNotifier(fd(), QSocketNotifier::Read, this);
		connect(m_capNotifier, SIGNAL(activated(int)), this, SLOT(capFrame()));
	}
}

void SupportedCamera::closeDevice()
{
	delete m_sigMapper;
	m_sigMapper = NULL;
	if (fd() >= 0) {
		if (m_capNotifier) {
			delete m_capNotifier;
			delete m_capImage;
			m_capNotifier = NULL;
			m_capImage = NULL;
		}
		delete m_frameData;
		m_frameData = NULL;
		v4lconvert_destroy(m_convertData);
		v4l2::close();
	}
    /*
    if(m_mainLayout != NULL) {
        while (QLayoutItem *item = m_mainLayout->takeAt(0)) {
            delete item->widget();
            delete item; // thanks stackoverflow
        }
    }*/
	m_ctrlMap.clear();
	m_widgetMap.clear();
	m_classMap.clear();
}

void SupportedCamera::error(const QString &error)
{
	if (!error.isEmpty())
        qDebug() << "V42L Error: " << error;
}

void SupportedCamera::error(int err)
{
	error(QString("Error: %1").arg(strerror(err)));
}

void SupportedCamera::errorCtrl(unsigned id, int err)
{
	error(QString("Error %1: %2")
		.arg((const char *)m_ctrlMap[id].name).arg(strerror(err)));
}

void SupportedCamera::errorCtrl(unsigned id, int err, const QString &v)
{
	error(QString("Error %1 (%2): %3")
		.arg((const char *)m_ctrlMap[id].name).arg(v).arg(strerror(err)));
}

void SupportedCamera::errorCtrl(unsigned id, int err, long long v)
{
	error(QString("Error %1 (%2): %3")
		.arg((const char *)m_ctrlMap[id].name).arg(v).arg(strerror(err)));
}

