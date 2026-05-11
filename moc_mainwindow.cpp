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
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[36];
    char stringdata0[486];
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
QT_MOC_LITERAL(7, 69, 16), // "onStartDetection"
QT_MOC_LITERAL(8, 86, 14), // "onRemoveCamera"
QT_MOC_LITERAL(9, 101, 8), // "cameraId"
QT_MOC_LITERAL(10, 110, 12), // "onOpenFolder"
QT_MOC_LITERAL(11, 123, 13), // "onBrowseModel"
QT_MOC_LITERAL(12, 137, 11), // "onLoadModel"
QT_MOC_LITERAL(13, 149, 13), // "onReloadModel"
QT_MOC_LITERAL(14, 163, 16), // "onStopProcessing"
QT_MOC_LITERAL(15, 180, 22), // "onConfThresholdChanged"
QT_MOC_LITERAL(16, 203, 5), // "value"
QT_MOC_LITERAL(17, 209, 21), // "onNmsThresholdChanged"
QT_MOC_LITERAL(18, 231, 23), // "onBatchInferenceToggled"
QT_MOC_LITERAL(19, 255, 10), // "onSettings"
QT_MOC_LITERAL(20, 266, 16), // "onFrameProcessed"
QT_MOC_LITERAL(21, 283, 5), // "image"
QT_MOC_LITERAL(22, 289, 22), // "std::vector<Detection>"
QT_MOC_LITERAL(23, 312, 10), // "detections"
QT_MOC_LITERAL(24, 323, 9), // "elapsedMs"
QT_MOC_LITERAL(25, 333, 16), // "onWorkerFinished"
QT_MOC_LITERAL(26, 350, 13), // "onWorkerError"
QT_MOC_LITERAL(27, 364, 7), // "message"
QT_MOC_LITERAL(28, 372, 12), // "onAlertSaved"
QT_MOC_LITERAL(29, 385, 9), // "videoPath"
QT_MOC_LITERAL(30, 395, 9), // "imagePath"
QT_MOC_LITERAL(31, 405, 9), // "alertJson"
QT_MOC_LITERAL(32, 415, 16), // "onStartRecording"
QT_MOC_LITERAL(33, 432, 15), // "onStopRecording"
QT_MOC_LITERAL(34, 448, 16), // "onViewRecordings"
QT_MOC_LITERAL(35, 465, 20) // "onClearOldRecordings"

    },
    "MainWindow\0onOpenImage\0\0onOpenVideo\0"
    "onOpenCamera\0checked\0onAddCamera\0"
    "onStartDetection\0onRemoveCamera\0"
    "cameraId\0onOpenFolder\0onBrowseModel\0"
    "onLoadModel\0onReloadModel\0onStopProcessing\0"
    "onConfThresholdChanged\0value\0"
    "onNmsThresholdChanged\0onBatchInferenceToggled\0"
    "onSettings\0onFrameProcessed\0image\0"
    "std::vector<Detection>\0detections\0"
    "elapsedMs\0onWorkerFinished\0onWorkerError\0"
    "message\0onAlertSaved\0videoPath\0imagePath\0"
    "alertJson\0onStartRecording\0onStopRecording\0"
    "onViewRecordings\0onClearOldRecordings"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,  129,    2, 0x08 /* Private */,
       3,    0,  130,    2, 0x08 /* Private */,
       4,    1,  131,    2, 0x08 /* Private */,
       6,    0,  134,    2, 0x08 /* Private */,
       7,    0,  135,    2, 0x08 /* Private */,
       8,    1,  136,    2, 0x08 /* Private */,
      10,    0,  139,    2, 0x08 /* Private */,
      11,    0,  140,    2, 0x08 /* Private */,
      12,    0,  141,    2, 0x08 /* Private */,
      13,    0,  142,    2, 0x08 /* Private */,
      14,    0,  143,    2, 0x08 /* Private */,
      15,    1,  144,    2, 0x08 /* Private */,
      17,    1,  147,    2, 0x08 /* Private */,
      18,    1,  150,    2, 0x08 /* Private */,
      19,    0,  153,    2, 0x08 /* Private */,
      20,    4,  154,    2, 0x08 /* Private */,
      25,    1,  163,    2, 0x08 /* Private */,
      26,    2,  166,    2, 0x08 /* Private */,
      28,    4,  171,    2, 0x08 /* Private */,
      32,    0,  180,    2, 0x08 /* Private */,
      33,    0,  181,    2, 0x08 /* Private */,
      34,    0,  182,    2, 0x08 /* Private */,
      35,    0,  183,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void, QMetaType::Int,   16,
    QMetaType::Void, QMetaType::Bool,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::QImage, 0x80000000 | 22, QMetaType::Double,    9,   21,   23,   24,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    9,   27,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString, QMetaType::QString,    9,   29,   30,   31,
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
        case 4: _t->onStartDetection(); break;
        case 5: _t->onRemoveCamera((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 6: _t->onOpenFolder(); break;
        case 7: _t->onBrowseModel(); break;
        case 8: _t->onLoadModel(); break;
        case 9: _t->onReloadModel(); break;
        case 10: _t->onStopProcessing(); break;
        case 11: _t->onConfThresholdChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->onNmsThresholdChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->onBatchInferenceToggled((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 14: _t->onSettings(); break;
        case 15: _t->onFrameProcessed((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2])),(*reinterpret_cast< std::vector<Detection>(*)>(_a[3])),(*reinterpret_cast< double(*)>(_a[4]))); break;
        case 16: _t->onWorkerFinished((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 17: _t->onWorkerError((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2]))); break;
        case 18: _t->onAlertSaved((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< const QString(*)>(_a[2])),(*reinterpret_cast< const QString(*)>(_a[3])),(*reinterpret_cast< const QString(*)>(_a[4]))); break;
        case 19: _t->onStartRecording(); break;
        case 20: _t->onStopRecording(); break;
        case 21: _t->onViewRecordings(); break;
        case 22: _t->onClearOldRecordings(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 15:
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
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
