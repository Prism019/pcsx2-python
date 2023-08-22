#pragma once

#include "PythonIncludeGuard.h"

#include <atomic>

namespace Python {

// a python type that wraps a set of callback functions, allowing easy dispatch
// and enabling/disabling of events
typedef struct {
    PyObject_HEAD
    PyObject* set;
    std::atomic_bool enabled;
} EventObject;

extern PyTypeObject EventType;

void Module_Init();

void Module_Destroy();

}