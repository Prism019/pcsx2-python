#include "Console.h"

#include "common/Console.h"

namespace Python::PyConsole {
    static PyObject* WriteLn(PyObject* self, PyObject* args) {
        const char* print_string;
        if(!PyArg_ParseTuple(args, "s", &print_string)) {
            return NULL;
        }
        Console.WriteLn("Python: %s", print_string);
        Py_RETURN_NONE;
    }

    static PyMethodDef Methods[] = {
        {"write_ln", WriteLn, METH_VARARGS, "Write a string to the console."},
        {NULL, NULL, 0, NULL},
    };

    struct PyModuleDef Module {
        PyModuleDef_HEAD_INIT,
        "pcsx2.console",
        NULL,
        -1,
        Methods,
    };

    PyObject* Init() {
        return PyModule_Create(&Module);
    }

    void Destroy() {}
}