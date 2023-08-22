#include "PythonModule.h"

#include "submodules/Console.h"
#include "submodules/GS.h"
#include "submodules/VM.h"

#include "common/Console.h"

namespace Python {

void Event_dealloc(EventObject* const self) {
    Py_XDECREF(self->set);
    Py_TYPE(self)->tp_free(self);
}

PyObject* Event_new(PyTypeObject* const type, PyObject* const args, PyObject* const kwds) {
    auto const self = (EventObject*) type->tp_alloc(type, 0);
    if(self == NULL) {
        return NULL;
    }
    self->set = PySet_New(NULL);
    if(self->set == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    self->enabled.store(false, std::memory_order_release);
    return (PyObject*) self;
}

int Event_init(EventObject* const self, PyObject* const args, PyObject* const kwds) {
    return 0;
}

PyObject* Event_add(EventObject* const self, PyObject* const args) {
    PyObject* callback;
    if(PyArg_ParseTuple(args, "O", &callback) < 0) {
        //failed to parse arguments... hmm
        Console.Error("Event::add - failed to parse arguments.");
        return NULL;
    }
    // first, we want to check if the function we're being passed is callable
    if(!PyCallable_Check(callback)) {
        // not callable, don't add and error out
        Console.Error("Event::add - callback is not callable.");
        return NULL;
    }
    // now we add the function, setting `.enabled` to true.
    PySet_Add(self->set, callback);
    self->enabled.store(true, std::memory_order_release);
    Py_RETURN_NONE;
}

PyObject* Event_remove(EventObject* const self, PyObject* const args) {
    PyObject* callback;
    if(PyArg_ParseTuple(args, "O", &callback) < 0) {
        // failed to parse arguments... hmm
        Console.Error("Event::remove - failed to parse arguments.");
        return NULL;
    }
    // remove the callback from the set
    PySet_Discard(self->set, callback);
    if(PySet_Size(self->set) == 0) {
        self->enabled.store(false, std::memory_order_release);
    }
    Py_RETURN_NONE;
}

PyObject* Event_clear(EventObject* const self, PyObject* const Py_UNUSED(args)) {
    PySet_Clear(self->set);
    self->enabled.store(false, std::memory_order_release);
    Py_RETURN_NONE;
}

const static PyMethodDef Event_methods[] = {
    {"add", (PyCFunction)Event_add, METH_VARARGS, NULL},
    {"remove", (PyCFunction)Event_remove, METH_VARARGS, NULL},
    {"clear", (PyCFunction)Event_clear, METH_NOARGS, NULL},
    {},
};

PyTypeObject EventType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pcsx2.Event",
    sizeof(EventObject),
    0,
    (destructor)Event_dealloc, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, NULL, NULL, NULL, NULL, 0, NULL, NULL,
    (PyMethodDef*)Event_methods, NULL, NULL, NULL, NULL, NULL, NULL, 0,
    (initproc)Event_init, NULL,
    Event_new, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL, NULL,
};

static struct PyModuleDef Module {
    PyModuleDef_HEAD_INIT,
    "pcsx2",
    NULL,
    -1,
};

void Module_Init() {
    if(PyType_Ready(&EventType) < 0) {
        Console.Error("Python: Event type not ready.");
        return;
    }

    auto main_module = PyModule_Create(&Module);
    auto module_dict = PyImport_GetModuleDict();
    
    if(PyModule_AddObjectRef(main_module, "Event", (PyObject*)&EventType) < 0) {
        Console.Error("Python: Failed to add Event to pcsx2 module.");
        Py_DECREF(main_module);
        return;
    }

    shutdown_event.Attach(main_module, "shutdown_event");

    //pcsx2.console
    auto console_module = PyConsole::Init();
    PyDict_SetItemString(module_dict, "pcsx2.console", console_module);
    PyModule_AddObject(main_module, "console", console_module);

    //pcsx2.gs
    auto gs_module = PyGS::Init();
    PyDict_SetItemString(module_dict, "pcsx2.gs", gs_module);
    PyModule_AddObject(main_module, "gs", gs_module);

    //pcsx2.vm
    auto vm_module = PyVM::Init();
    PyDict_SetItemString(module_dict, "pcsx2.vm", vm_module);
    PyModule_AddObject(main_module, "vm", vm_module);

    PyDict_SetItemString(module_dict, "pcsx2", main_module);
}

void Module_Destroy() {
    PyVM::Destroy();
    PyGS::Destroy();
    PyConsole::Destroy();

    shutdown_event.Detach();
}

}