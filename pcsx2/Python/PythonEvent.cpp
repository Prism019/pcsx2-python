#include "PythonEvent.h"

#include <string>
#include "common/Console.h"

namespace Python {

Event::Event() {
    this->id = id_counter++;
}

void Event::Attach(PyObject *module, const char *name) {
    auto args = PyTuple_New(0);
    auto py_event = PyObject_CallObject((PyObject*)&EventType, args);
    this->py_event = (EventObject*)py_event;
    PyModule_AddObjectRef(module, name, py_event);
    Py_DECREF(args);
}

void Event::Detach() {
    Py_DECREF(py_event);
}

u64 Event::GetId() {
    return id;
}

EventObject* Event::GetEvent() {
    return py_event;
}

void WriteData(u64* data, u64 t) {
    *data = t;
}
void WriteData(u64* data, long long t) {
    *data = static_cast<u64>(t);
}
void WriteData(u64* data, const char* t) {
    // pass a new string and delete it from the other side
    // to ensure it's still there when we get to it
    auto buf = new char[std::strlen(t) + 1];
    std::strcpy(buf, t);
    *data = reinterpret_cast<u64>(buf);
}

std::unordered_map<u64, Event*> event_map;

u64 Event::id_counter = 3;

template<>
PyObject* MakePyObject<u64>(u64* data) {
    return PyLong_FromUnsignedLongLong(*data);
}
template<>
PyObject* MakePyObject<long long>(u64* data) {
    return PyLong_FromLongLong(*data);
}
template<>
PyObject* MakePyObject<const char*>(u64* data) {
    // make sure to delete the string we allocated
    // in the corresponding WriteData
    auto str = reinterpret_cast<char*>(*data);
    auto py_str = PyUnicode_FromString(str);
    delete str;
    return py_str;
}

SpecializedEvent<void>::SpecializedEvent() : Event() {
    event_map.insert({GetId(), this});
}

void SpecializedEvent<void>::Invoke() {
    if(GetEvent()->enabled.load(std::memory_order_acquire) == false) return;
    PythonThread::WriteData(GetId());
}

bool SpecializedEvent<void>::Dispatch() {
    auto GIL = PyGILState_Ensure();
    auto set_copy = PySet_New(GetEvent()->set);
    auto iter = PyObject_GetIter(set_copy);
    PyObject* callback;
    auto args = PyTuple_New(0);
    while((callback = PyIter_Next(iter))) {
        if(!PyCallable_Check(callback)) {
            Console.Error("Python: Not callable.");
            Py_DECREF(callback);
            Py_DECREF(iter);
            Py_DECREF(args);
            Py_DECREF(set_copy);
            return false;
        }
        auto return_value = PyObject_CallObject(callback, args);
        Py_XDECREF(return_value);
        Py_DECREF(callback);
    }
    Py_DECREF(iter);
    Py_DECREF(args);
    Py_DECREF(set_copy);
    PyGILState_Release(GIL);
    return true;
}

SpecializedEvent<void> shutdown_event;

}