#pragma once

#include "Python/PythonIncludeGuard.h"

#include "Python/PythonEvent.h"

namespace Python::PyVM {
    extern void (*SetVMPaused)(bool paused);
    extern bool (*GetVMPaused)();
    extern std::string (*GetCurrentDiscSerial)();
    extern SpecializedEvent<void> started_event;

    PyObject* Init();
    void Destroy();
}