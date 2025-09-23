//
// Created by dan on 25-1-7.
//

#ifndef TOOLS_H
#define TOOLS_H

#include <mutex>
#include <deque>
#include <string>
#include <thread>
#include <chrono>
#include <functional>
#include <condition_variable>

#include "ThreadLog.h"

#define LOCK_TILL_EXIT_SCOPE() \
    static std::timed_mutex mtx; \
    std::unique_lock<std::timed_mutex> lock(mtx, std::defer_lock); \
    while (!lock.try_lock_for(std::chrono::seconds(1))) {  \
        LOG_WARN("wait lock timeout!!!"); \
    }

#define RETURN_IF_NOT_FIRST_ENTRANT() LOCK_TILL_EXIT_SCOPE() static bool first_entrant=true;if(first_entrant){first_entrant=false;}else{return;}

inline int to_int(const std::string &str) {
    if (str.empty())
        return 0;
    return std::stoi(str);
}

inline std::string to_string(const std::string &str) {
    return str;
}

inline std::string to_string(const char *chars) {
    return chars;
}

inline void sleep_sec(unsigned int seconds) {
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
}

inline void sleep_millisec(unsigned int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

template<typename T>
std::string to_string(const T &t) {
    return std::to_string(t);
}

template<typename T>
std::string combine(const T &t) {
    return to_string(t);
}

template<typename First, typename... Rest>
std::string combine(const First &first, const Rest &... rest) {
    return to_string(first) + ',' + combine(rest...);
}

class ConditionVariable {
    std::mutex mtx;
    bool ready = false;
    std::condition_variable cv;

public:
    void wait_for_ever() {
        std::unique_lock lock(mtx);
        LOG_CALL();

        ready = false;

        while (!ready)
            cv.wait(lock);
    }

    void wait(uint sec = 20) {
        std::unique_lock lock(mtx);
        LOG_CALL(sec);

        ready = false;

        auto end_time = std::chrono::system_clock::now() + std::chrono::seconds(sec);
        while (!ready) {
            if (cv.wait_until(lock, end_time) == std::cv_status::timeout) {
                LOG_ERROR("wait cv time out");
                break;
            }
        }
    }

    void call_then_wait(std::function<void()> func, uint sec = 60) {
        std::unique_lock lock(mtx);
        LOG_CALL(sec);

        ready = false;

        func();

        auto end_time = std::chrono::system_clock::now() + std::chrono::seconds(sec);
        while (!ready) {
            if (cv.wait_until(lock, end_time) == std::cv_status::timeout) {
                LOG_ERROR("wait cv time out");
                break;
            }
        }
    }

    void notify() {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
        cv.notify_one();
    }
};

template<typename T>
class MessageQueue {
public:
    explicit MessageQueue(size_t size_limit = 1) : m_size_limit(size_limit) {
    }

    void block_push(const T &t) {
        std::unique_lock lk(m_mtx);

        m_push_cv.wait(lk,
                       [this] {
                           return m_queue.size() < m_size_limit;
                       });

        m_queue.push_back(t);
        m_pop_cv.notify_one();
    }

    void block_pop(T &t) {
        std::unique_lock lk(m_mtx);

        m_pop_cv.wait(lk,
                      [this] {
                          return !m_queue.empty();
                      });
        t = std::move(m_queue.front());
        m_queue.pop_front();
        m_push_cv.notify_one();
    }

private:
    std::mutex m_mtx;
    size_t m_size_limit;
    std::deque<T> m_queue;
    std::condition_variable m_pop_cv;
    std::condition_variable m_push_cv;
};


std::string get_field(const std::string &str, int field_index, const std::string &delimiter = ",");

bool run_cmd(const std::string &cmd);

bool run_cmd(const std::string &cmd, std::string *run_result);

bool untar(const std::string &tar_file_path,
           const std::string &untar_directory);

// untar and remove one or more top level folders
bool untar_and_rm_top_folders(const std::string &tar_file_path,
                              const std::string &untar_directory,
                              int rm_top_folders_num);

bool umount(const std::string &partition_or_dir);

bool mount_partition_to_dir(const std::string &partition, const std::string &dir);

std::string dir_mounted_partition(const std::string &dir);

bool clear_dir(const std::string &dir);

bool file_exists(const std::string &path);

bool mkdir(const std::string &dir);

bool mkfile(const std::string &dir);

bool rm_path(const std::string &path);

bool contains_sub_str(const std::string &str, const std::string &sub_str);

bool contains_str(const std::string &file_path, const std::string &str);

bool rm_if_exists_mk_if_not(const std::string &file_path);

bool append_env_var(const std::string &env_var, const std::string &append_value);

std::string get_file_content(const std::string &path);

std::string get_line(const std::string &str, size_t line_number);

bool verify_signature(const std::string &file_path,
                      const std::string &signature_path,
                      const std::string &public_key);

bool starts_with(const std::string &str, const std::string &prefix);

int get_system_memory_size_kb();

bool change_file_name(const std::string &old_path, const std::string &new_path);
#endif //TOOLS_H
