#pragma once

#include "PythonIncludeGuard.h"

#include <atomic>

#include "common/Pcsx2Defs.h"
#include "common/Threading.h"

namespace Python {
const u64 EVT_RESET = 1;
const u64 EVT_LOAD_SCRIPT = 2;

class PythonThread {
    static const s32 buffer_size = (_1mb) / sizeof(u64);
    u64 work[buffer_size];
    alignas(64) std::atomic<size_t> m_ato_read_pos;
    alignas(64) std::atomic<size_t> m_ato_write_pos;
    alignas(64) size_t m_read_pos;
    size_t m_write_pos;
    std::mutex m_writing_mutex;

    Threading::WorkSema m_work_sema;
    std::atomic_bool m_shutdown_flag{false};
    Threading::Thread m_thread;

public:
    PythonThread();
    ~PythonThread();

    static void start();
    static void stop();

    void Open();
    void Close();
    void Kick();

    static u64 ReadData();
    static void ReadData(u64* const data, size_t count);
    static void WriteData(const u64 data);
    static void WriteData(const u64* const data, size_t count);

    static void OpenScript(std::string path);

private:
    void Process();

    size_t GetReadPos();
    size_t GetWritePos();
    void CommitReadPos();
    void CommitWritePos();

    void ReserveSpace(size_t size);
    void WaitOnSize(size_t size);

    u64 Read();

    void Write(u64 data);
};
}