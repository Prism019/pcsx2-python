#include "PythonThread.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <fstream>

#include "common/Console.h"
#include "common/Threading.h"
#include "PythonEvent.h"
#include "PythonModule.h"

namespace Python {
PythonThread* g_python_thread = nullptr;

PythonThread::PythonThread() {
    Console.WriteLn("Python thread created!");
    // initialize Python interpreter
    Py_InitializeEx(0);
    Module_Init();
    Console.WriteLn("Python version %s initialised!", Py_GetVersion());

    if(PyGILState_Check()) {
        PyEval_SaveThread();
    }
}

PythonThread::~PythonThread() {
    Module_Destroy();
    PyGILState_Ensure();
    if(Py_FinalizeEx() != 0) {
        PyErr_Print();
    }
    Console.WriteLn("Python thread destroyed!");
}

void PythonThread::Open() {
    Console.WriteLn("Python thread opened!");
    m_work_sema.Reset();
    m_thread.Start([this]() {Process();});
    Kick();
}

void PythonThread::Close() {
    Console.WriteLn("Python thread closed!");
    m_shutdown_flag.store(true, std::memory_order_release);
    Kick();
    m_thread.Join();
}

void PythonThread::Kick() {
    m_work_sema.NotifyOfWork();
}

void PythonThread::start() {
    g_python_thread = new PythonThread();
    g_python_thread->Open();
}

void PythonThread::stop() {
    g_python_thread->Close();
    delete g_python_thread;
    g_python_thread = nullptr;
}

void PythonThread::Process() {
    Threading::SetNameOfCurrentThread("Python Worker");
    Console.WriteLn("Python thread started!");

    for(;;) {
        m_work_sema.WaitForWork();
        while(m_ato_read_pos.load(std::memory_order_relaxed) != GetWritePos()) {
            auto event_id = Read();
            if(event_id == EVT_RESET) {
                m_read_pos = 0;
            } else if(event_id == EVT_LOAD_SCRIPT) {
                // read the path from the work buffer
                auto path = reinterpret_cast<char*>(Read());
                // open the path and pass it to python
                auto file = std::fopen(path, "rb");
                auto GIL = PyGILState_Ensure();
                PyRun_SimpleFileEx(file, path, true);
                PyGILState_Release(GIL);
                delete[] path;
            } else {
                auto event = event_map[event_id];
                event->Dispatch();
            }
            CommitReadPos();
        }

        if(m_shutdown_flag.load(std::memory_order_acquire)) {
            break;
        }
    }
}

size_t PythonThread::GetReadPos() {
    return m_ato_read_pos.load();
}

size_t PythonThread::GetWritePos() {
    return m_ato_write_pos.load();
}

void PythonThread::CommitReadPos() {
    m_ato_read_pos.store(m_read_pos);
}

void PythonThread::CommitWritePos() {
    m_ato_write_pos.store(m_write_pos);
}

void PythonThread::WaitOnSize(size_t size) {
    for (;;) {
        auto read_pos = GetReadPos();
        if(read_pos <= m_write_pos) {
            break;
        }
        if(read_pos > m_write_pos + size) {
            break;
        }
        Kick();
        std::this_thread::yield();
    }
}

void PythonThread::ReserveSpace(size_t size) {
    if(m_write_pos + size > (buffer_size - 1)) {
        WaitOnSize(1);
        Write(EVT_RESET);
        m_write_pos = 0;
    }

    WaitOnSize(size);
}

u64 PythonThread::Read() {
    auto value = work[m_read_pos];
    m_read_pos++;
    return value;
}

u64 PythonThread::ReadData() {
    auto value = g_python_thread->Read();
    g_python_thread->CommitReadPos();
    return value;
}

void PythonThread::ReadData(u64* const data, size_t count) {
    for(size_t i = 0; i < count; i++) {
        data[i] = g_python_thread->Read();
    }
    g_python_thread->CommitReadPos();
}

void PythonThread::Write(u64 data) {
    work[m_write_pos] = data;
    m_write_pos++;
}

void PythonThread::WriteData(const u64 data) {
    {
        auto guard = std::lock_guard(g_python_thread->m_writing_mutex);
        g_python_thread->ReserveSpace(1);
        g_python_thread->Write(data);
        g_python_thread->CommitWritePos();
    }
    g_python_thread->Kick();
}

void PythonThread::WriteData(const u64* const data, size_t len) {
    {
        auto guard = std::lock_guard(g_python_thread->m_writing_mutex);
        g_python_thread->ReserveSpace(len);
        for(size_t i = 0; i < len; i++) {
            g_python_thread->Write(data[i]);
        }
        g_python_thread->CommitWritePos();
    }
    g_python_thread->Kick();
}

void PythonThread::OpenScript(std::string path) {
    // make a new char buffer and copy the path into it
    auto buf = new char[path.length() + 1];
    std::strcpy(buf, path.c_str());
    // add an event to open the script
    {
        auto guard = std::lock_guard(g_python_thread->m_writing_mutex);
        g_python_thread->ReserveSpace(2);
        g_python_thread->Write(EVT_LOAD_SCRIPT);
        g_python_thread->Write(reinterpret_cast<u64>(buf));
        g_python_thread->CommitWritePos();
    }
    g_python_thread->Kick();
}
}
