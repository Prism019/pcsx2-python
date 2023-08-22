#pragma once

// ensure that Qt doesn't interfere with python
#undef slots

#define PY_SSIZE_T_CLEAN
#include <Python.h>