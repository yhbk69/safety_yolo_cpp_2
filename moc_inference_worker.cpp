/****************************************************************************
** Meta object code from reading C++ file 'inference_worker.hpp'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "inference_worker.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'inference_worker.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_InferenceWorker_t {
    QByteArrayData data[18];
    char stringdata0[194];
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
QT_MOC_LITERAL(15, 163, 4), // "stop"
QT_MOC_LITERAL(16, 168, 17), // "setBatchInference"
QT_MOC_LITERAL(17, 186, 7) // "enabled"

    },
    "InferenceWorker\0frameProcessed\0\0"
    "cameraId\0image\0std::vector<Detection>\0"
    "detections\0elapsedMs\0alertSaved\0"
    "videoPath\0imagePath\0alertJson\0finished\0"
    "errorOccurred\0message\0stop\0setBatchInference\0"
    "enabled"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_InferenceWorker[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    4,   44,    2, 0x06 /* Public */,
       8,    4,   53,    2, 0x06 /* Public */,
      12,    1,   62,    2, 0x06 /* Public */,
      13,    2,   65,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      15,    0,   70,    2, 0x0a /* Public */,
      16,    1,   71,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int, QMetaType::QImage, 0x80000000 | 5, QMetaType::Double,    3,    4,    6,    7,
    QMetaType::Void, QMetaType::Int, QMetaType::QString, QMetaType::QString, QMetaType::QString,    3,    9,   10,   11,
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int, QMetaType::QString,    3,   14,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   17,

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
        case 4: _t->stop(); break;
        case 5: _t->setBatchInference((*reinterpret_cast< bool(*)>(_a[1]))); break;
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
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
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
QT_WARNING_POP
QT_END_MOC_NAMESPACE
