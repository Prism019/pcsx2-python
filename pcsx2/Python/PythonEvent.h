#pragma once

#include <atomic>
#include <numeric>
#include <unordered_map>

#include "PythonThread.h"
#include "PythonModule.h"
#include "common/Console.h"

namespace Python {
class Event {
    static u64 id_counter;
    u64 id;
    EventObject* py_event;
public:
    Event();
    void Attach(PyObject* module, const char* name);
    void Detach();
    u64 GetId();
    EventObject* GetEvent();
    virtual bool Dispatch() = 0;
};

extern std::unordered_map<u64, Event*> event_map;

template<typename T>
constexpr size_t SizeOfType = (sizeof(T) + 7) / sizeof(u64);
template<typename... Args>
constexpr size_t SizeOfDataCalculate() {
    size_t sizes[] = {SizeOfType<Args>...};
    return std::accumulate(std::begin(sizes), std::end(sizes), 0);
}
template<typename... Args>
constexpr size_t SizeOfData = SizeOfDataCalculate<Args...>();

void WriteData(u64* data, u64 t);
void WriteData(u64* data, long long t);
void WriteData(u64* data, const char* t);
template<typename... Args>
void BuildData(u64* data, Args... args) {
    auto build_ptr = data;
    ([&] {
        WriteData(build_ptr, args);
        build_ptr += SizeOfData<Args>;
    }(), ...);
}


template<typename T>
PyObject* MakePyObject(u64* data) = delete;
template<>
PyObject* MakePyObject<u64>(u64* data);
template<>
PyObject* MakePyObject<long long>(u64* data);
template<>
PyObject* MakePyObject<const char*>(u64* data);
template<typename... Args>
PyObject* MakePyTuple(u64* data) {
    auto data_offset = data;
    auto tuple = PyTuple_New((ssize_t)sizeof...(Args));
    ssize_t element = 0;
    ([&] {
        auto element_object = MakePyObject<Args>(data_offset);
        PyTuple_SetItem(tuple, element, element_object);
        data_offset += SizeOfType<Args>;
        element++;
    }(), ...);
    return tuple;
}

template<typename... Args>
class SpecializedEvent final : public Event {
private:
public:
    SpecializedEvent() : Event() {
        event_map.insert({GetId(), this});
    };
    // called from outside the python thread
    void Invoke(Args... args){
        if(GetEvent()->enabled.load(std::memory_order_acquire) == false) return;
        const auto size_of_data = SizeOfData<Args...> + 1;
        u64 data[size_of_data];
        data[0] = GetId();
        BuildData(&data[1], args...);
        PythonThread::WriteData(data, size_of_data);
    }
    // called from inside the python thread
    bool Dispatch() override {
        auto GIL = PyGILState_Ensure();
        auto set_copy = PySet_New(GetEvent()->set);
        const auto size_of_data = SizeOfData<Args...>;
        u64 data[size_of_data];
        PythonThread::ReadData(data, size_of_data);
        // build python tuple
        auto args = MakePyTuple<Args...>(data);
        // call the functions!
        auto iter = PyObject_GetIter(set_copy);
        PyObject* callback;
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
    };
};

template<>
class SpecializedEvent<void> final : public Event {
public:
    SpecializedEvent();
    // called from outside the python thread
    void Invoke();
    // called from inside the python thread
    bool Dispatch() override;
};

extern SpecializedEvent<void> shutdown_event;

}