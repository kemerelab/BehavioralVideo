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


#include "general-tab.h"

#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleValidator>

#include <QDebug>

#include <stdio.h>
#include <errno.h>

GeneralTab::GeneralTab(const QString &device, v4l2 &fd, int n, QWidget *parent) :
	QGridLayout(parent),
	v4l2(fd),
	m_row(0),
	m_col(0),
	m_cols(n),
	m_isRadio(false),
    m_vidCapFormats(NULL),
	m_frameSize(NULL),
    m_vidOutFormats(NULL)
{
	setSpacing(3);

	setSizeConstraint(QLayout::SetMinimumSize);

	if (querycap(m_querycap)) {
		addLabel("Device:");
		addLabel(device + (useWrapper() ? " (wrapped)" : ""), Qt::AlignLeft);

		addLabel("Driver:");
		addLabel((char *)m_querycap.driver, Qt::AlignLeft);

		addLabel("Card:");
		addLabel((char *)m_querycap.card, Qt::AlignLeft);

		addLabel("Bus:");
		addLabel((char *)m_querycap.bus_info, Qt::AlignLeft);
	}

	g_tuner(m_tuner);
	g_modulator(m_modulator);

	v4l2_input vin;

	if (m_tuner.capability && m_tuner.capability & V4L2_TUNER_CAP_LOW)
		m_isRadio = true;
	if (m_modulator.capability && m_modulator.capability & V4L2_TUNER_CAP_LOW)
		m_isRadio = true;
	if (m_querycap.capabilities & V4L2_CAP_DEVICE_CAPS)
		m_isVbi = caps() & (V4L2_CAP_VBI_CAPTURE | V4L2_CAP_SLICED_VBI_CAPTURE);

	if (!isRadio() && enum_input(vin, true)) {
		addLabel("Input");
		m_videoInput = new QComboBox(parent);
		do {
			m_videoInput->addItem((char *)vin.name);
		} while (enum_input(vin));
		addWidget(m_videoInput);
		connect(m_videoInput, SIGNAL(activated(int)), SLOT(inputChanged(int)));
	}

	v4l2_fmtdesc fmt;
	addLabel("Capture Image Formats");
	m_vidCapFormats = new QComboBox(parent);
	if (enum_fmt_cap(fmt, true)) {
		do {
			QString s(pixfmt2s(fmt.pixelformat) + " (");

			if (fmt.flags & V4L2_FMT_FLAG_EMULATED)
				m_vidCapFormats->addItem(s + "Emulated)");
			else
				m_vidCapFormats->addItem(s + (const char *)fmt.description + ")");
		} while (enum_fmt_cap(fmt));
	}
	addWidget(m_vidCapFormats);
	connect(m_vidCapFormats, SIGNAL(activated(int)), SLOT(vidCapFormatChanged(int)));

	addLabel("Frame Width");
	m_frameWidth = new QSpinBox(parent);
	addWidget(m_frameWidth);
	connect(m_frameWidth, SIGNAL(editingFinished()), SLOT(frameWidthChanged()));
	addLabel("Frame Height");
	m_frameHeight = new QSpinBox(parent);
	addWidget(m_frameHeight);
	connect(m_frameHeight, SIGNAL(editingFinished()), SLOT(frameHeightChanged()));

	addLabel("Frame Size");
	m_frameSize = new QComboBox(parent);
	m_frameSize->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	addWidget(m_frameSize);
	connect(m_frameSize, SIGNAL(activated(int)), SLOT(frameSizeChanged(int)));

	addLabel("Frame Interval");
	m_frameInterval = new QComboBox(parent);
	m_frameInterval->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	addWidget(m_frameInterval);
	connect(m_frameInterval, SIGNAL(activated(int)), SLOT(frameIntervalChanged(int)));

	updateVidCapFormat();

	if (caps() & V4L2_CAP_VIDEO_OUTPUT) {
		addLabel("Output Image Formats");
		m_vidOutFormats = new QComboBox(parent);
		if (enum_fmt_out(fmt, true)) {
			do {
				m_vidOutFormats->addItem(pixfmt2s(fmt.pixelformat) +
						" - " + (const char *)fmt.description);
			} while (enum_fmt_out(fmt));
		}
		addWidget(m_vidOutFormats);
		connect(m_vidOutFormats, SIGNAL(activated(int)), SLOT(vidOutFormatChanged(int)));
	}

	addLabel("Capture Method");
	m_capMethods = new QComboBox(parent);
    m_buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (caps() & V4L2_CAP_STREAMING) {
		v4l2_requestbuffers reqbuf;

		// Yuck. The videobuf framework does not accept a count of 0.
		// This is out-of-spec, but it means that the only way to test which
		// method is supported is to give it a non-zero count. But non-videobuf
		// drivers like uvc do not allow e.g. S_FMT calls after a REQBUFS call
		// with non-zero counts unless there is a REQBUFS call with count == 0
		// in between. This is actual proper behavior, although somewhat
		// unexpected. So the only way at the moment to do this that works
		// everywhere is to call REQBUFS with a count of 1, and then again with
		// a count of 0.
		if (reqbufs_user(reqbuf, 1)) {
			m_capMethods->addItem("User pointer I/O", QVariant(methodUser));
			reqbufs_user(reqbuf, 0);
		}
		if (reqbufs_mmap(reqbuf, 1)) {
			m_capMethods->addItem("Memory mapped I/O", QVariant(methodMmap));
			reqbufs_mmap(reqbuf, 0);
		}
	}
	if (caps() & V4L2_CAP_READWRITE) {
		m_capMethods->addItem("read()", QVariant(methodRead));
	}
	addWidget(m_capMethods);

}

void GeneralTab::addWidget(QWidget *w, Qt::Alignment align)
{
	QGridLayout::addWidget(w, m_row, m_col, align | Qt::AlignVCenter);
	m_col++;
	if (m_col == m_cols) {
		m_col = 0;
		m_row++;
	}
}

CapMethod GeneralTab::capMethod()
{
	return (CapMethod)m_capMethods->itemData(m_capMethods->currentIndex()).toInt();
}

void GeneralTab::inputChanged(int input)
{
	s_input(input);
}


void GeneralTab::vidCapFormatChanged(int idx)
{
	v4l2_fmtdesc desc;

	enum_fmt_cap(desc, true, idx);

	v4l2_format fmt;

	g_fmt_cap(fmt);
	fmt.fmt.pix.pixelformat = desc.pixelformat;
	if (try_fmt(fmt))
		s_fmt(fmt);

	updateVidCapFormat();
}

void GeneralTab::frameWidthChanged()
{
	v4l2_format fmt;
	int val = m_frameWidth->value();

	g_fmt_cap(fmt);
	fmt.fmt.pix.width = val;
	if (try_fmt(fmt))
		s_fmt(fmt);

	updateVidCapFormat();
}

void GeneralTab::frameHeightChanged()
{
	v4l2_format fmt;
	int val = m_frameHeight->value();

	g_fmt_cap(fmt);
	fmt.fmt.pix.height = val;
	if (try_fmt(fmt))
		s_fmt(fmt);

	updateVidCapFormat();
}

void GeneralTab::frameSizeChanged(int idx)
{
	v4l2_frmsizeenum frmsize;

	if (enum_framesizes(frmsize, m_pixelformat, idx)) {
		v4l2_format fmt;

		g_fmt_cap(fmt);
		fmt.fmt.pix.width = frmsize.discrete.width;
		fmt.fmt.pix.height = frmsize.discrete.height;
		if (try_fmt(fmt))
			s_fmt(fmt);
	}
	updateVidCapFormat();
}

void GeneralTab::frameIntervalChanged(int idx)
{
	v4l2_frmivalenum frmival;

	if (enum_frameintervals(frmival, m_pixelformat, m_width, m_height, idx)
	    && frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE) {
		if (set_interval(frmival.discrete))
			m_interval = frmival.discrete;
	}
	updateVidCapFormat();
}

void GeneralTab::updateVidCapFormat()
{
	v4l2_fmtdesc desc;
	v4l2_format fmt;

	g_fmt_cap(fmt);
	m_pixelformat = fmt.fmt.pix.pixelformat;
	m_width       = fmt.fmt.pix.width;
	m_height      = fmt.fmt.pix.height;
	updateFrameSize();
	updateFrameInterval();
	if (enum_fmt_cap(desc, true)) {
		do {
			if (desc.pixelformat == fmt.fmt.pix.pixelformat)
				break;
		} while (enum_fmt_cap(desc));
	}
	if (desc.pixelformat != fmt.fmt.pix.pixelformat)
		return;
	m_vidCapFormats->setCurrentIndex(desc.index);
}

void GeneralTab::updateFrameSize()
{
	v4l2_frmsizeenum frmsize;
	bool ok = false;

	m_frameSize->clear();

	ok = enum_framesizes(frmsize, m_pixelformat);
	if (ok && frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
		do {
			m_frameSize->addItem(QString("%1x%2")
				.arg(frmsize.discrete.width).arg(frmsize.discrete.height));
			if (frmsize.discrete.width == m_width &&
			    frmsize.discrete.height == m_height)
				m_frameSize->setCurrentIndex(frmsize.index);
		} while (enum_framesizes(frmsize));

		m_frameWidth->setEnabled(false);
		m_frameHeight->setEnabled(false);
		m_frameWidth->setMinimum(m_width);
		m_frameWidth->setMaximum(m_width);
		m_frameWidth->setValue(m_width);
		m_frameHeight->setMinimum(m_height);
		m_frameHeight->setMaximum(m_height);
		m_frameHeight->setValue(m_height);
		m_frameSize->setEnabled(true);
		updateFrameInterval();
		return;
	}
	if (!ok) {
		frmsize.stepwise.min_width = 8;
		frmsize.stepwise.max_width = 1920;
		frmsize.stepwise.step_width = 1;
		frmsize.stepwise.min_height = 8;
		frmsize.stepwise.max_height = 1200;
		frmsize.stepwise.step_height = 1;
	}
	m_frameWidth->setEnabled(true);
	m_frameHeight->setEnabled(true);
	m_frameSize->setEnabled(false);
	m_frameWidth->setMinimum(frmsize.stepwise.min_width);
	m_frameWidth->setMaximum(frmsize.stepwise.max_width);
	m_frameWidth->setSingleStep(frmsize.stepwise.step_width);
	m_frameWidth->setValue(m_width);
	m_frameHeight->setMinimum(frmsize.stepwise.min_height);
	m_frameHeight->setMaximum(frmsize.stepwise.max_height);
	m_frameHeight->setSingleStep(frmsize.stepwise.step_height);
	m_frameHeight->setValue(m_height);
	updateFrameInterval();
}

void GeneralTab::updateFrameInterval()
{
	v4l2_frmivalenum frmival;
	v4l2_fract curr;
	bool curr_ok, ok;

	m_frameInterval->clear();

	ok = enum_frameintervals(frmival, m_pixelformat, m_width, m_height);
	m_has_interval = ok && frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE;
	m_frameInterval->setEnabled(m_has_interval);
	if (m_has_interval) {
	        m_interval = frmival.discrete;
        	curr_ok = v4l2::get_interval(curr);
		do {
			m_frameInterval->addItem(QString("%1 fps")
				.arg((double)frmival.discrete.denominator / frmival.discrete.numerator));
			if (curr_ok &&
			    frmival.discrete.numerator == curr.numerator &&
			    frmival.discrete.denominator == curr.denominator) {
				m_frameInterval->setCurrentIndex(frmival.index);
				m_interval = frmival.discrete;
                        }
		} while (enum_frameintervals(frmival));
	}
}

bool GeneralTab::get_interval(struct v4l2_fract &interval)
{
	if (m_has_interval)
		interval = m_interval;

	return m_has_interval;
}
