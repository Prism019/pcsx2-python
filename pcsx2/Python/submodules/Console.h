#pragma once

#include "Python/PythonIncludeGuard.h"

namespace Python::PyConsole {
    PyObject* Init();
    void Destroy();
}