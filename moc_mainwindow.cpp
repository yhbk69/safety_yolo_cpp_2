/****************************************************************************
** Meta object code from reading C++ file 'mainwindow.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "mainwindow.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mainwindow.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_InferenceWorker_t {
    QByteArrayData data[24];
    char stringdata0[261];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_InferenceWorker_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_InferenceWorker_t qt_meta_stringdata_InferenceWorker = {
    {
QT_MOC_LITERAL(0, 0, 15), // "InferenceWorker"
QT_MOC_LITERAL(1, 16, 14), // "frameProcessed"
QT_MOC_LITERAL(2, 31, 0), // ""
QT_MOC_LITERAL(3, 32, 8), // "cameraId"
QT_MOC_LITERAL(4, 41, 5), // "image"
QT_MOC_LITERAL(5, 47, 22), // "std::vector<Detection>"
QT_MOC_LITERAL(6, 70, 10), // "detections"
QT_MOC_LITERAL(7, 81, 9), // "elapsedMs"
QT_MOC_LITERAL(8, 91, 10), // "alertSaved"
QT_MOC_LITERAL(9, 102, 9), // "videoPath"
QT_MOC_LITERAL(10, 112, 9), // "imagePath"
QT_MOC_LITERAL(11, 122, 9), // "alertJson"
QT_MOC_LITERAL(12, 132, 8), // "finished"
QT_MOC_LITERAL(13, 141, 13), // "errorOccurred"
QT_MOC_LITERAL(14, 155, 7), // "message"
QT_MOC_LITERAL(15, 163, 12), // "processVideo"
QT_MOC_LITERAL(16, 176, 4), // "path"
QT_MOC_LITERAL(17, 181, 10), // "confThresh"
QT_MOC_LITERAL(18, 192, 9), // "nmsThresh"
QT_MOC_LITERAL(19, 202, 13), // "processCamera"
QT_MOC_LITERAL(20, 216, 13), // "processSource"
QT_MOC_LITERAL(21, 230, 4), // "stop"
QT_MOC_LITERAL(22, 235, 17), // "setBatchInference"
QT_MOC_LITERAL(23, 253, 7) // "enabled"

    },
    "InferenceWorker\0frameProcessed\0\0"
    "cameraId\0image\0std::vector<Detection>\0"
    "detections\0elapsedMs\0alertSaved\0"
    "videoPath\0imagePath\0alertJson\0finished\0"
    "errorOccurred\0message\0processVideo\0"
    "path\0confThresh\0nmsThresh\0processCamera\0"
    "processSource\0stop\0setBatchInference\0"
    "enabled"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_InferenceWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    4,   59,    2, 0x06 /* Public */,
       8,    4,   68,    2, 0x06 /* Public */,
      12,    1,   77,    2, 0x06 /* Public */,
      13,    2,   80,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    3,   85,    2, 0x0a /* Public */,
      19,    2,   92,    2, 0x0a /* Public */,
      20,    2,   97,    2, 0x0a /* Public */,
      21,    0,  102,    2, 0x0a /* Public */,
      22,    1,  103,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QImage, 0x80000000 | 5, QMetaType::Double,    3,    4,    6,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString, QMetaType::QString,    3,    9,   10,   11,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,   14,

 // slots: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::Float, QMetaType::Float,   16,   17,   18,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   17,   18,
    QMetaType::Void, QMetaType::Float, QMetaType::Float,   17,   18,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   23,

       0        // eod
};

void InferenceWorker::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<InferenceWorker *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->frameProcessed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2])),(*reinterpret_cast< std::vector<Detection>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4]))); break;
        case 1: _t->alertSaved((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2])),(*reinterpret_cast< QString(*)>(_a[3])),(*reinterpret_cast< QString(*)>(_a[4]))); break;
        case 2: _t->finished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->errorOccurred((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 4: _t->processVideo((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2])),(*reinterpret_cast< float(*)>(_a[3]))); break;
        case 5: _t->processCamera((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 6: _t->processSource((*reinterpret_cast< float(*)>(_a[1])),(*reinterpret_cast< float(*)>(_a[2]))); break;
        case 7: _t->stop(); break;
        case 8: _t->setBatchInference((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 2:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< std::vector<Detection> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (InferenceWorker::*)(int , QImage , std::vector<Detection> , double );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceWorker::frameProcessed)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (InferenceWorker::*)(int , QString , QString , QString );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceWorker::alertSaved)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (InferenceWorker::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceWorker::finished)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (InferenceWorker::*)(int , const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&InferenceWorker::errorOccurred)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject InferenceWorker::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_InferenceWorker.data,
    qt_meta_data_InferenceWorker,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *InferenceWorker::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *InferenceWorker::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_InferenceWorker.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int InferenceWorker::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void InferenceWorker::frameProcessed(int _t1, QImage _t2, std::vector<Detection> _t3, double _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void InferenceWorker::alertSaved(int _t1, QString _t2, QString _t3, QString _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void InferenceWorker::finished(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void InferenceWorker::errorOccurred(int _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[46];
    char stringdata0[618];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 11), // "onOpenImage"
QT_MOC_LITERAL(2, 23, 0), // ""
QT_MOC_LITERAL(3, 24, 11), // "onOpenVideo"
QT_MOC_LITERAL(4, 36, 12), // "onOpenCamera"
QT_MOC_LITERAL(5, 49, 7), // "checked"
QT_MOC_LITERAL(6, 57, 11), // "onAddCamera"
QT_MOC_LITERAL(7, 69, 14), // "onRemoveCamera"
QT_MOC_LITERAL(8, 84, 8), // "cameraId"
QT_MOC_LITERAL(9, 93, 12), // "onOpenFolder"
QT_MOC_LITERAL(10, 106, 13), // "onBrowseModel"
QT_MOC_LITERAL(11, 120, 11), // "onLoadModel"
QT_MOC_LITERAL(12, 132, 13), // "onReloadModel"
QT_MOC_LITERAL(13, 146, 16), // "onStopProcessing"
QT_MOC_LITERAL(14, 163, 22), // "onConfThresholdChanged"
QT_MOC_LITERAL(15, 186, 5), // "value"
QT_MOC_LITERAL(16, 192, 21), // "onNmsThresholdChanged"
QT_MOC_LITERAL(17, 214, 23), // "onBatchInferenceToggled"
QT_MOC_LITERAL(18, 238, 10), // "onSettings"
QT_MOC_LITERAL(19, 249, 16), // "onFrameProcessed"
QT_MOC_LITERAL(20, 266, 5), // "image"
QT_MOC_LITERAL(21, 272, 22), // "std::vector<Detection>"
QT_MOC_LITERAL(22, 295, 10), // "detections"
QT_MOC_LITERAL(23, 306, 9), // "elapsedMs"
QT_MOC_LITERAL(24, 316, 16), // "onWorkerFinished"
QT_MOC_LITERAL(25, 333, 13), // "onWorkerError"
QT_MOC_LITERAL(26, 347, 7), // "message"
QT_MOC_LITERAL(27, 355, 12), // "onAlertSaved"
QT_MOC_LITERAL(28, 368, 9), // "videoPath"
QT_MOC_LITERAL(29, 378, 9), // "imagePath"
QT_MOC_LITERAL(30, 388, 9), // "alertJson"
QT_MOC_LITERAL(31, 398, 19), // "onWsClientConnected"
QT_MOC_LITERAL(32, 418, 15), // "onWsTextMessage"
QT_MOC_LITERAL(33, 434, 10), // "retryAlarm"
QT_MOC_LITERAL(34, 445, 7), // "alarmId"
QT_MOC_LITERAL(35, 453, 17), // "handleSyncRequest"
QT_MOC_LITERAL(36, 471, 11), // "lastAlarmId"
QT_MOC_LITERAL(37, 483, 16), // "handleGetStreams"
QT_MOC_LITERAL(38, 500, 14), // "handleSetFence"
QT_MOC_LITERAL(39, 515, 8), // "streamId"
QT_MOC_LITERAL(40, 524, 5), // "fence"
QT_MOC_LITERAL(41, 530, 16), // "handleViewStream"
QT_MOC_LITERAL(42, 547, 16), // "onStartRecording"
QT_MOC_LITERAL(43, 564, 15), // "onStopRecording"
QT_MOC_LITERAL(44, 580, 16), // "onViewRecordings"
QT_MOC_LITERAL(45, 597, 20) // "onClearOldRecordings"

    },
    "MainWindow\0onOpenImage\0\0onOpenVideo\0"
    "onOpenCamera\0checked\0onAddCamera\0"
    "onRemoveCamera\0cameraId\0onOpenFolder\0"
    "onBrowseModel\0onLoadModel\0onReloadModel\0"
    "onStopProcessing\0onConfThresholdChanged\0"
    "value\0onNmsThresholdChanged\0"
    "onBatchInferenceToggled\0onSettings\0"
    "onFrameProcessed\0image\0std::vector<Detection>\0"
    "detections\0elapsedMs\0onWorkerFinished\0"
    "onWorkerError\0message\0onAlertSaved\0"
    "videoPath\0imagePath\0alertJson\0"
    "onWsClientConnected\0onWsTextMessage\0"
    "retryAlarm\0alarmId\0handleSyncRequest\0"
    "lastAlarmId\0handleGetStreams\0"
    "handleSetFence\0streamId\0fence\0"
    "handleViewStream\0onStartRecording\0"
    "onStopRecording\0onViewRecordings\0"
    "onClearOldRecordings"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      29,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  159,    2, 0x08 /* Private */,
       3,    0,  160,    2, 0x08 /* Private */,
       4,    1,  161,    2, 0x08 /* Private */,
       6,    0,  164,    2, 0x08 /* Private */,
       7,    1,  165,    2, 0x08 /* Private */,
       9,    0,  168,    2, 0x08 /* Private */,
      10,    0,  169,    2, 0x08 /* Private */,
      11,    0,  170,    2, 0x08 /* Private */,
      12,    0,  171,    2, 0x08 /* Private */,
      13,    0,  172,    2, 0x08 /* Private */,
      14,    1,  173,    2, 0x08 /* Private */,
      16,    1,  176,    2, 0x08 /* Private */,
      17,    1,  179,    2, 0x08 /* Private */,
      18,    0,  182,    2, 0x08 /* Private */,
      19,    4,  183,    2, 0x08 /* Private */,
      24,    1,  192,    2, 0x08 /* Private */,
      25,    2,  195,    2, 0x08 /* Private */,
      27,    4,  200,    2, 0x08 /* Private */,
      31,    0,  209,    2, 0x08 /* Private */,
      32,    1,  210,    2, 0x08 /* Private */,
      33,    1,  213,    2, 0x08 /* Private */,
      35,    1,  216,    2, 0x08 /* Private */,
      37,    0,  219,    2, 0x08 /* Private */,
      38,    2,  220,    2, 0x08 /* Private */,
      41,    1,  225,    2, 0x08 /* Private */,
      42,    0,  228,    2, 0x08 /* Private */,
      43,    0,  229,    2, 0x08 /* Private */,
      44,    0,  230,    2, 0x08 /* Private */,
      45,    0,  231,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Int,   15,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QImage, 0x80000000 | 21, QMetaType::Double,    8,   20,   22,   23,
    QMetaType::Void, QMetaType::Int,    8,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    8,   26,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString, QMetaType::QString,    8,   28,   29,   30,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,   26,
    QMetaType::Void, QMetaType::QString,   34,
    QMetaType::Void, QMetaType::QString,   36,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QJsonObject,   39,   40,
    QMetaType::Void, QMetaType::QString,   39,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->onOpenImage(); break;
        case 1: _t->onOpenVideo(); break;
        case 2: _t->onOpenCamera((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 3: _t->onAddCamera(); break;
        case 4: _t->onRemoveCamera((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->onOpenFolder(); break;
        case 6: _t->onBrowseModel(); break;
        case 7: _t->onLoadModel(); break;
        case 8: _t->onReloadModel(); break;
        case 9: _t->onStopProcessing(); break;
        case 10: _t->onConfThresholdChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->onNmsThresholdChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->onBatchInferenceToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 13: _t->onSettings(); break;
        case 14: _t->onFrameProcessed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2])),(*reinterpret_cast< std::vector<Detection>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4]))); break;
        case 15: _t->onWorkerFinished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 16: _t->onWorkerError((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 17: _t->onAlertSaved((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        case 18: _t->onWsClientConnected(); break;
        case 19: _t->onWsTextMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 20: _t->retryAlarm((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 21: _t->handleSyncRequest((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 22: _t->handleGetStreams(); break;
        case 23: _t->handleSetFence((*reinterpret_cast< const QString(*)>(_a[1])),(*reinterpret_cast< const QJsonObject(*)>(_a[2]))); break;
        case 24: _t->handleViewStream((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 25: _t->onStartRecording(); break;
        case 26: _t->onStopRecording(); break;
        case 27: _t->onViewRecordings(); break;
        case 28: _t->onClearOldRecordings(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 14:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 2:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< std::vector<Detection> >(); break;
            }
            break;
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 29)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 29;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
