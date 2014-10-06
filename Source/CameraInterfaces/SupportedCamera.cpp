#include "SupportedCamera.h"
#include <QDir>
#include <QDebug>

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

#include "SupportedCamera.h"
#include "V4L2/general-tab.h"
#include <assert.h>
#include <sys/mman.h>
#include <errno.h>
#include <dirent.h>
#include <libv4l2.h>


SupportedCamera::SupportedCamera(QString name, QTabWidget *prefTabs, QObject *parent) :
    GenericCameraInterface(parent)
{
    if (QFile::exists(name))
        device = name;

    cameraName = QString("V4L2 ") + device;
    prefPanel = prefTabs;

    m_capNotifier = NULL;
    m_capImage = NULL;
    m_frameData = NULL;
    m_nbuffers = 0;
    m_buffers = NULL;
    m_sigMapper = NULL;

    m_mainLayout = NULL;

    //connect(openRawAct, SIGNAL(triggered()), this, SLOT(openrawdev()));
    //m_capStartAct->setDisabled(true);
    //connect(m_capStartAct, SIGNAL(toggled(bool)), this, SLOT(capStart(bool)));
    //connect(closeAct, SIGNAL(triggered()), this, SLOT(closeDevice()));
    setDevice(device, true);
}

SupportedCamera::~SupportedCamera()
{
    closeDevice();
}

void SupportedCamera::Initialize()
{
}

void SupportedCamera::StartCapture(bool enableStrobe)
{
    capStart(true);
}

void SupportedCamera::StopCapture()
{
    capStart(false);
}

void FindSupportedCameras(QStringList *cameraNameList) {
    QDir dir("/dev","video*",QDir::Name,QDir::Files | QDir::System);
    for (int i = 0; i < dir.entryInfoList().length(); i++)
        cameraNameList->append(dir.entryInfoList().at(i).absoluteFilePath());
}
