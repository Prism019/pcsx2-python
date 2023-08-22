#pragma once

#include "Python/PythonIncludeGuard.h"

#include "Python/PythonEvent.h"

namespace Python::PyGS {
    extern SpecializedEvent<void> vsync_event;

    PyObject* Init();
    void Destroy();
}