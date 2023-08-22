#include "VM.h"

#include <condition_variable>
#include <mutex>

#include "common/Pcsx2Types.h"
#include "Memory.h"

namespace Python::PyVM {
    std::string (*GetCurrentDiscSerial)() = nullptr;
    void (*SetVMPaused)(bool paused) = nullptr;
    bool (*GetVMPaused)() = nullptr;
    SpecializedEvent<void> started_event;

    static PyObject* SetPaused(PyObject* self, PyObject* args) {
        if(SetVMPaused == nullptr) {
            Console.Error("SetVMPaused function pointer not set! Bailing!");
            return NULL;
        }
        int paused;
        if(!PyArg_ParseTuple(args, "p", &paused)) {
            return NULL;
        }
        SetVMPaused(paused);
        Py_RETURN_NONE;
    }

    static PyObject* GetPaused(PyObject* self, PyObject* args) {
        if(GetVMPaused == nullptr) {
            Console.Error("GetVMPaused function pointer not set! Bailing!");
            return NULL;
        }
        bool paused = GetVMPaused();
        if (paused) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    }

    static PyObject* CurrentDiscSerial(PyObject* self, PyObject* args) {
        if(GetCurrentDiscSerial == nullptr) {
            Console.Error("GetCurrentDiscSerial function pointer not set! Bailing!");
            return NULL;
        }
        return PyUnicode_FromString(GetCurrentDiscSerial().c_str());
    }

    static PyObject* ReadU8(PyObject* self, PyObject* args) {
        u32 addr;
        if(!PyArg_ParseTuple(args, "I", &addr)) {
            return NULL;
        }
        return PyLong_FromLong(memRead8(addr));
    }
    static PyObject* ReadU16(PyObject* self, PyObject* args) {
        u32 addr;
        if(!PyArg_ParseTuple(args, "I", &addr)) {
            return NULL;
        }
        return PyLong_FromLong(memRead16(addr));
    }
    static PyObject* ReadU32(PyObject* self, PyObject* args) {
        u32 addr;
        if(!PyArg_ParseTuple(args, "I", &addr)) {
            return NULL;
        }
        return PyLong_FromLong(memRead32(addr));
    }
    static PyObject* ReadU64(PyObject* self, PyObject* args) {
        u32 addr;
        if(!PyArg_ParseTuple(args, "I", &addr)) {
            return NULL;
        }
        return PyLong_FromUnsignedLongLong(memRead64(addr));
    }
    static PyObject* WriteU8(PyObject* self, PyObject* args) {
        u32 addr;
        u8 value;
        if(!PyArg_ParseTuple(args, "Ib", &addr, &value)) {
            return NULL;
        }
        memWrite8(addr, value);
        Py_RETURN_NONE;
    }
    static PyObject* WriteU16(PyObject* self, PyObject* args) {
        u32 addr;
        u16 value;
        if(!PyArg_ParseTuple(args, "Ih", &addr, &value)) {
            return NULL;
        }
        memWrite16(addr, value);
        Py_RETURN_NONE;
    }
    static PyObject* WriteU32(PyObject* self, PyObject* args) {
        u32 addr;
        u32 value;
        if(!PyArg_ParseTuple(args, "II", &addr, &value)) {
            return NULL;
        }
        memWrite32(addr, value);
        Py_RETURN_NONE;
    }
    static PyObject* WriteU64(PyObject* self, PyObject* args) {
        u32 addr;
        u64 value;
        if(!PyArg_ParseTuple(args, "IK", &addr, &value)) {
            return NULL;
        }
        memWrite64(addr, value);
        Py_RETURN_NONE;
    }

    static PyMethodDef Methods[] = {
        {"set_paused", SetPaused, METH_VARARGS, "Set the paused state of the VM."},
        {"get_paused", GetPaused, METH_NOARGS, "Get the paused state of the VM."},
        {"current_disc_serial", CurrentDiscSerial, METH_NOARGS, "Get the serial of the currently-loaded disc."},
        {"read_u8", ReadU8, METH_VARARGS, "Read an 8-bit value from the specified address."},
        {"read_u16", ReadU16, METH_VARARGS, "Read a 16-bit value from the specified address."},
        {"read_u32", ReadU32, METH_VARARGS, "Read a 32-bit value from the specified address."},
        {"read_u64", ReadU64, METH_VARARGS, "Read a 64-bit value from the specified address."},
        {"write_u8", WriteU8, METH_VARARGS, "Write an 8-bit value to the specified address."},
        {"write_u16", WriteU16, METH_VARARGS, "Write a 16-bit value to the specified address."},
        {"write_u32", WriteU32, METH_VARARGS, "Write a 32-bit value to the specified address."},
        {"write_u64", WriteU64, METH_VARARGS, "Write a 64-bit value to the specified address."},
        {NULL, NULL, 0, NULL},
    };

    static struct PyModuleDef Module {
        PyModuleDef_HEAD_INIT,
        "pcsx2.vm",
        NULL,
        -1,
        Methods,
    };
    PyObject* Init() {
        auto module = PyModule_Create(&Module);
        started_event.Attach(module, "started_event");
        return module;
    }
    void Destroy() {
        started_event.Detach();
    }
}