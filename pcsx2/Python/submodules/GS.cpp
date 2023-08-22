#include "GS.h"

namespace Python::PyGS {
    SpecializedEvent<void> vsync_event;

    static struct PyModuleDef Module {
        PyModuleDef_HEAD_INIT,
        "pcsx2.gs",
        NULL,
        -1,
    };
    PyObject* Init() {
        auto module = PyModule_Create(&Module);
        vsync_event.Attach(module, "vsync_event");
        return module;
    }
    void Destroy() {
        vsync_event.Detach();
    }
}