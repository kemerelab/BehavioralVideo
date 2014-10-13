#ifndef SUPPORTEDCAMERA_H
#define SUPPORTEDCAMERA_H

#include <QObject>
#include <QWidget>
//#include <QCamera>
#include <QSignalMapper>
#include <QSocketNotifier>
#include <QGridLayout>
#include <QLabel>
#include <QVideoFrame>
#include "GenericCamera.h"
#include "V4L2/v4l2-api.h"

class GeneralTab;

typedef std::vector<unsigned> ClassIDVec;
typedef std::map<unsigned, ClassIDVec> ClassMap;
typedef std::map<unsigned, struct v4l2_queryctrl> CtrlMap;
typedef std::map<unsigned, QWidget *> WidgetMap;

enum {
    CTRL_UPDATE_ON_CHANGE = 0x10,
    CTRL_DEFAULTS,
    CTRL_REFRESH,
    CTRL_UPDATE
};

enum CapMethod {
    methodRead,
    methodMmap,
    methodUser
};

struct buffer {
    void   *start;
    size_t  length;
};

#define CTRL_FLAG_DISABLED	(V4L2_CTRL_FLAG_READ_ONLY | \
                 V4L2_CTRL_FLAG_INACTIVE | \
                 V4L2_CTRL_FLAG_GRABBED)


void FindSupportedCameras(QStringList *cameraNameList);

class SupportedCamera : public GenericCameraInterface, public v4l2
{
    Q_OBJECT
public:
    explicit SupportedCamera(QString device, QTabWidget *prefTabs, QObject *parent = 0);
    ~SupportedCamera(void);

private slots:
    void setDevice(const QString &device, bool rawOpen);
    void closeDevice();
    void capStart(bool);
    void capFrame();
    void ctrlAction(int id);


public:
    virtual void error(const QString &text);
    void error(int err);
    void errorCtrl(unsigned id, int err);
    void errorCtrl(unsigned id, int err, long long v);
    void errorCtrl(unsigned id, int err, const QString &v);
    void info(const QString &info);

private:
    bool initiateCapture(unsigned buffer_size);
    void haltCapture();
    void addControlWidgets();
    void addWidget(QGridLayout *grid, QWidget *w, Qt::Alignment align);
    void finishGrid(QGridLayout *grid, unsigned ctrl_class);
    void addLabel(QGridLayout *grid, const QString &text, Qt::Alignment align = Qt::AlignRight)
    {
 //     addWidget(grid, new QLabel(text, parentWidget()), align);
        addWidget(grid, new QLabel(text, grid->parentWidget()), align);
    }
    void addCtrl(QGridLayout *grid, const struct v4l2_queryctrl &qctrl);
    void updateCtrl(unsigned id);
    void refresh(unsigned ctrl_class);
    void refresh();
    void setDefaults(unsigned ctrl_class);
    int getVal(unsigned id);
    long long getVal64(unsigned id);
    QString getString(unsigned id);
    void setVal(unsigned id, int v);
    void setVal64(unsigned id, long long v);
    void setString(unsigned id, const QString &v);
    QString getCtrlFlags(unsigned flags);
    void setWhat(QWidget *w, unsigned id, const QString &v);
    void setWhat(QWidget *w, unsigned id, long long v);
    void updateVideoInput();

    QString device;
    struct buffer *m_buffers;
    struct v4l2_format m_capSrcFormat;
    struct v4l2_format m_capDestFormat;
    unsigned char *m_frameData;
    unsigned m_nbuffers;
    struct v4lconvert_data *m_convertData;
    bool m_mustConvert;
    CapMethod m_capMethod;
    QSignalMapper *m_sigMapper;
    QSocketNotifier *m_capNotifier;
    //QImage *m_capImage;
    QVideoFrame *m_capImage;
    int m_row, m_col, m_cols;
    CtrlMap m_ctrlMap;
    WidgetMap m_widgetMap;
    ClassMap m_classMap;
    bool m_haveExtendedUserCtrls;
    unsigned m_frame;
    unsigned m_lastFrame;
    unsigned m_fps;
    struct timeval m_tv;

    GeneralTab *m_genTab;
    QTabWidget *prefPanel;
    QWidget *m_tabs;
    QGridLayout *m_mainLayout;

signals:

public slots:
    void Initialize();
    void StartCapture(bool enableStrobe);
    void StopCapture(void);

};

#endif // SUPPORTEDCAMERA_H
