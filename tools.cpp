//
// Created by dan on 25-1-7.
//

#include <array>
#include <regex>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#include "ThreadTraceLog.h"

std::string get_field(const std::string& str, int field_index, const std::string& delimiter) {
    using namespace std;
    regex del(delimiter);

    sregex_token_iterator it(str.begin(), str.end(), del, -1);
    sregex_token_iterator end;

    int i = 0;
    while (it != end) {
        if (i == field_index) {
            return *it;
        }
        ++i;
        ++it;
    }
    return "";
}

bool run_cmd(const std::string& cmd, std::string* run_result) {
    log_call(cmd);

    auto* pipe = popen(cmd.c_str(), "r");
    if (pipe == nullptr) {
        log_error("popen() failed, run cmd %s failed", cmd.c_str());
        return false;
    }

    *run_result = "";
    std::array<char, 128> buf{};
    while (fgets(buf.data(), buf.size(), pipe) != nullptr) {
        *run_result += static_cast<std::string>(buf.data());
    }

    auto exit_code = pclose(pipe);
    if (WIFEXITED(exit_code)) {
        exit_code = WEXITSTATUS(exit_code);
        if (exit_code == 0) {
            return true;
        }
        log_error("run cmd failed, exit code: %d", exit_code);
    } else {
        log_error("run cmd failed");
    }

    return false;
}

bool run_cmd(const std::string& cmd) {
    std::string no_use;
    return run_cmd(cmd, &no_use);
}

bool untar(const std::string& tar_file_path,
           const std::string& untar_directory) {
    log_call(tar_file_path, untar_directory);

    return run_cmd("tar -xzvf " + tar_file_path + " -C " + untar_directory);
}

// untar and remove first level folder to avoid folder overlap
bool untar_and_rm_top_folders(const std::string& tar_file_path,
                              const std::string& untar_directory,
                              int rm_top_folders_num) {
    log_call(tar_file_path, untar_directory, rm_top_folders_num);

    return run_cmd("tar -xzvf " + tar_file_path + " -C " + untar_directory +
        " --strip-components=" + std::to_string(rm_top_folders_num));
}

bool umount(const std::string& partition_or_dir) {
    log_call(partition_or_dir);

    return run_cmd("umount " + partition_or_dir);
}

bool mount_partition_to_dir(const std::string& partition, const std::string& dir) {
    log_call(partition, dir);

    return run_cmd("mount " + partition + " " + dir);
}

std::string dir_mounted_partition(const std::string& dir) {
    log_call(dir);

    std::string cmd = "df -h | grep " + dir + " | awk '{print $1}' | tr -d '[:space:]'";

    std::string ret;
    if (run_cmd(cmd, &ret)) {
        return ret;
    }
    return "";
}

bool clear_dir(const std::string& dir) {
    log_call(dir);
    return run_cmd("rm -rf " + dir + "/*");
}

bool file_exists(const std::string& path) {
    log_call(path);
    if (access(path.c_str(), F_OK) != -1) {
        log_info("file exists");
        return true;
    }
    log_info("file not exists");
    return false;
}

bool mkdir(const std::string& dir) {
    log_call(dir);
    return run_cmd("mkdir -p " + dir);
}

bool mkfile(const std::string& dir) {
    log_call(dir);
    return run_cmd("touch " + dir);
}

bool overwrite_file(const std::string& path, const std::string& content) {
    log_call(path, content);

    std::ofstream file_stream(path, std::ios::out | std::ios::trunc);
    if (!file_stream.is_open()) {
        log_error("open file failed!");
        return false;
    }

    file_stream << content;
    if (!file_stream) {
        log_error("write content failed!");
        return false;
    }

    file_stream.flush();
    if (!file_stream) {
        log_error("flush file failed!");
        return false;
    }

    file_stream.close();
    if (!file_stream) {
        log_error("close file failed!");
        return false;
    }

    return true;
}

bool fsync_file(const std::string& path) {
    log_call(path);

    int fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        log_error("open file failed!");
        return false;
    }

    bool ret = true;
    if (fsync(fd) < 0) {
        log_error("fsync file failed!");
        ret = false;
    }

    int close_ret = close(fd);
    if (close_ret < 0) {
        log_error("close file failed!");
        ret = false;
    }

    return ret;
}

bool rm_path(const std::string& path) {
    log_call(path);
    return run_cmd("rm -rf " + path);
}

bool contains_sub_str(const std::string& str, const std::string& sub_str) {
    return str.find(sub_str) != std::string::npos;
}

bool contains_str(const std::string& file_path, const std::string& str) {
    log_call(file_path, str);

    std::ifstream file(file_path);
    if (!file.is_open()) {
        log_error("Open file failed!!!");
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(str) != std::string::npos) {
            file.close();
            return true;
        }
    }

    log_info("File does not contain string!!!");
    file.close();
    return false;
}

bool rm_if_exists_mk_if_not(const std::string& file_path) {
    log_call(file_path);

    if (file_exists(file_path)) {
        if (!rm_path(file_path)) {
            return false;
        }
    } else {
        if (!mkfile(file_path)) {
            return false;
        }
    }
    return true;
}

bool append_env_var(const std::string& env_var, const std::string& append_value) {
    log_call(env_var, append_value);

    const char* current_value = getenv(env_var.c_str());
    std::string updated_value;

    if (current_value != nullptr) {
        updated_value = std::string(current_value) + ":" + append_value;
    } else {
        updated_value = append_value;
    }

    if (setenv(env_var.c_str(), updated_value.c_str(), 1) != 0) {
        log_error("setenv failed!!!");
        return false;
    }
    return true;
}

std::string get_file_content(const std::string& path) {
    log_call(path);

    std::string content;

    if (!run_cmd("cat " + path, &content)) {
        log_error("get content of %s failed!", path.c_str());
        return "";
    }
    return content.substr(0, content.length() - 1); // remove a extra '\n' of cat output
}

std::string get_line(const std::string& str, size_t line_number) {
    std::istringstream iss(str);
    std::string line;
    size_t i = 0;

    while (std::getline(iss, line)) {
        if (++i == line_number) {
            return line;
        }
    }

    return "";
}

bool verify_signature(const std::string& file_path,
                      const std::string& signature_path,
                      const std::string& public_key) {
    std::string cmd = "echo \"" + public_key +
        "\" | openssl dgst -sha256 -verify /dev/stdin -signature " +
        signature_path + " " + file_path;

    std::string result;
    run_cmd(cmd, &result);

    return contains_sub_str(result, "Verified OK");
}

bool starts_with(const std::string& str, const std::string& prefix) {
    return str.rfind(prefix, 0) == 0;
}

int get_system_memory_size_kb() {
    log_call();

    std::ifstream meminfo("/proc/meminfo");
    std::string line;

    while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
            std::istringstream iss(line);
            std::string key;
            std::string value;
            std::string unit;
            iss >> key >> value >> unit;

            log_info("system mem size: %s kb", value.c_str());

            try {
                return std::stoi(value);
            } catch (...) {
                break;
            }
        }
    }

    log_error("get system mem size failed!!!");
    return -1;
}

bool change_file_name(const std::string& old_path, const std::string& new_path) {
    std::string cmd = "mv " + old_path + " " + new_path;
    return run_cmd(cmd);
}

int64_t get_mono_sec() {
    timespec ts{};

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec;
}

// return CLOCK_MONOTONIC time (milliseconds)
int64_t get_mono_millisec() {
    timespec ts{};

    clock_gettime(CLOCK_MONOTONIC, &ts);

    const int64_t tv_sec = ts.tv_sec; // expand it's range to avoid future overflow
    const int64_t tv_nsec = ts.tv_nsec;

    return tv_sec * 1000 + tv_nsec / 1000'000;
}
