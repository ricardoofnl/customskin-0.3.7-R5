#include "model/download.h"

#include "core/log.h"
#include "model/crc32.h"

#include <windows.h>
#include <wininet.h>

#include <condition_variable>
#include <cstdio>
#include <deque>
#include <mutex>
#include <thread>

namespace dl {

bool Fetch(const std::string& url, const std::string& destPath) {
    // open.mp's webserver rejects anything whose user-agent isn't exactly "samp/0.3"
    HINTERNET hNet = InternetOpenA("SAMP/0.3", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hNet) {
        CS_LOGE("dl: InternetOpen failed (%lu)", GetLastError());
        return false;
    }

    HINTERNET hUrl = InternetOpenUrlA(hNet, url.c_str(), nullptr, 0,
                                      INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hUrl) {
        CS_LOGE("dl: InternetOpenUrl failed for %s (%lu)", url.c_str(), GetLastError());
        InternetCloseHandle(hNet);
        return false;
    }

    FILE* f = std::fopen(destPath.c_str(), "wb");
    if (!f) {
        CS_LOGE("dl: cannot open %s for writing", destPath.c_str());
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hNet);
        return false;
    }

    char buf[65536];
    DWORD read = 0;
    size_t total = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &read) && read > 0) {
        std::fwrite(buf, 1, read, f);
        total += read;
    }

    std::fclose(f);
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hNet);

    CS_LOGI("dl: %s -> %s (%zu bytes)", url.c_str(), destPath.c_str(), total);
    return total > 0;
}

namespace {

struct Job {
    std::string url;
    std::string dest;
    uint32_t crc = 0;
    bool isDff = false;
};

std::mutex g_mtx;                 // guards g_jobs and g_done
std::condition_variable g_cv;
std::deque<Job> g_jobs;           // pending downloads (worker consumes)
std::vector<Result> g_done;       // finished downloads (main thread drains)
std::once_flag g_workerOnce;

// single detached worker: fetches queued files serially and crc-checks each. it only touches the
// queues + its own file, never game state, so nothing is shared unsafely with the main thread
void WorkerLoop() {
    for (;;) {
        Job job;
        {
            std::unique_lock<std::mutex> lk(g_mtx);
            g_cv.wait(lk, [] { return !g_jobs.empty(); });
            job = g_jobs.front();
            g_jobs.pop_front();
        }
        Result r;
        r.crc = job.crc;
        r.isDff = job.isDff;
        if (Fetch(job.url, job.dest))
            r.ok = (crc::File(job.dest.c_str()) == job.crc);
        {
            std::lock_guard<std::mutex> lk(g_mtx);
            g_done.push_back(r);
        }
    }
}

} // namespace

void Enqueue(const std::string& url, const std::string& destPath, uint32_t crc, bool isDff) {
    std::call_once(g_workerOnce, [] { std::thread(WorkerLoop).detach(); });
    {
        std::lock_guard<std::mutex> lk(g_mtx);
        g_jobs.push_back(Job{ url, destPath, crc, isDff });
    }
    g_cv.notify_one();
}

std::vector<Result> Drain() {
    std::vector<Result> out;
    std::lock_guard<std::mutex> lk(g_mtx);
    out.swap(g_done);
    return out;
}

} // namespace dl
