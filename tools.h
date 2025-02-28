//
// Created by dan on 25-1-7.
//

#ifndef TOOLS_H
#define TOOLS_H

#include <mutex>
#include <string>
#include <functional>
#include <condition_variable>

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
    void wait() {
        std::unique_lock lock(mtx);
        ready = false;
        while (!ready) {
            cv.wait(lock);
        }
    }

    void wait_after(std::function<void()> func) {
        std::unique_lock lock(mtx);
        ready = false;
        func();
        while (!ready) {
            cv.wait(lock);
        }
    }

    void notify() {
        std::lock_guard<std::mutex> lock(mtx);
        ready = true;
        cv.notify_one();
    }
};

std::string get_field(const std::string &str, int field_index);

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

bool append_env_var(const std::string& env_var, const std::string& append_value);

std::string get_file_content(const std::string& path);

bool verify_signature(const std::string &file_path,
                      const std::string &signature_path,
                      const std::string &public_key);
#endif //TOOLS_H
