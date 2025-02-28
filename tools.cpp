//
// Created by dan on 25-1-7.
//

#include <array>
#include <regex>
#include <string>
#include <fstream>

#include "ThreadLog.h"

std::string get_field(const std::string &str, int field_index) {
    using namespace std;
    regex del(",");

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

bool run_cmd(const std::string &cmd, std::string *run_result) {
    LOG_CALL(cmd);

    auto pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("popen() failed, run cmd %s failed", cmd.c_str());
        return false;
    }

    *run_result = "";
    std::array<char, 128> buf{};
    while (fgets(buf.data(), buf.size(), pipe) != nullptr) {
        *run_result += buf.data();
    }

    auto exit_code = pclose(pipe);
    if (WIFEXITED(exit_code)) {
        exit_code = WEXITSTATUS(exit_code);
        if (exit_code == 0) {
            return true;
        }
        LOG_ERROR("run cmd failed, exit code: %d", exit_code);
    } else {
        LOG_ERROR("run cmd failed");
    }

    return false;
}

bool run_cmd(const std::string &cmd) {
    std::string no_use;
    return run_cmd(cmd, &no_use);
}

bool untar(const std::string &tar_file_path,
           const std::string &untar_directory) {
    LOG_CALL(tar_file_path, untar_directory);

    return run_cmd("tar -xzvf " + tar_file_path + " -C " + untar_directory);
}

// untar and remove first level folder to avoid folder overlap
bool untar_and_rm_top_folders(const std::string &tar_file_path,
                              const std::string &untar_directory,
                              int rm_top_folders_num) {
    LOG_CALL(tar_file_path, untar_directory, rm_top_folders_num);

    return run_cmd("tar -xzvf " + tar_file_path + " -C " + untar_directory +
                   " --strip-components=" + std::to_string(rm_top_folders_num));
}

bool umount(const std::string &partition_or_dir) {
    LOG_CALL(partition_or_dir);

    return run_cmd("umount " + partition_or_dir);
}

bool mount_partition_to_dir(const std::string &partition, const std::string &dir) {
    LOG_CALL(partition, dir);

    return run_cmd("mount " + partition + " " + dir);
}

std::string dir_mounted_partition(const std::string &dir) {
    LOG_CALL(dir);

    std::string cmd = "df -h | grep " + dir + " | awk '{print $1}' | tr -d '[:space:]'";

    std::string ret;
    if (run_cmd(cmd, &ret)) {
        return ret;
    }
    return "";
}

bool clear_dir(const std::string &dir) {
    LOG_CALL(dir);
    return run_cmd("rm -rf " + dir + "/*");
}

bool file_exists(const std::string &path) {
    LOG_CALL(path);
    if (access(path.c_str(), F_OK) != -1) {
        LOG_INFO("file exists");
        return true;
    }
    LOG_INFO("file not exists");
    return false;
}

bool mkdir(const std::string &dir) {
    LOG_CALL(dir);
    return run_cmd("mkdir -p " + dir);
}

bool mkfile(const std::string &dir) {
    LOG_CALL(dir);
    return run_cmd("touch " + dir);
}

bool rm_path(const std::string &path) {
    LOG_CALL(path);
    return run_cmd("rm -rf " + path);
}

bool contains_sub_str(const std::string &str, const std::string &sub_str) {
    return str.find(sub_str) != std::string::npos;
}

bool contains_str(const std::string &file_path, const std::string &str) {
    LOG_CALL(file_path, str);

    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR("Open file failed!!!");
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find(str) != std::string::npos) {
            file.close();
            return true;
        }
    }

    LOG_INFO("File does not contain string!!!");
    file.close();
    return false;
}

bool rm_if_exists_mk_if_not(const std::string &file_path) {
    LOG_CALL(file_path);

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

bool append_env_var(const std::string &env_var, const std::string &append_value) {
    LOG_CALL(env_var, append_value);

    const char *current_value = getenv(env_var.c_str());
    std::string updated_value;

    if (current_value != nullptr) {
        updated_value = std::string(current_value) + ":" + append_value;
    } else {
        updated_value = append_value;
    }

    if (setenv(env_var.c_str(), updated_value.c_str(), 1) != 0) {
        LOG_ERROR("setenv failed!!!");
        return false;
    }
    return true;
}

std::string get_file_content(const std::string &path) {
    LOG_CALL(path);

    std::string content;

    if (!run_cmd("cat " + path, &content)) {
        LOG_ERROR("get content of %s failed!", path.c_str());
        return "";
    }
    return content.substr(0, content.length() - 1); // remove a extra '\n' of cat output
}

bool verify_signature(const std::string &file_path,
                      const std::string &signature_path,
                      const std::string &public_key) {
    std::string public_key_path = "/tmp/public_key.pem";
    std::ofstream tmp_file(public_key_path);
    tmp_file << public_key;
    tmp_file.close();

    std::string cmd = "openssl dgst -sha256 -verify " +
                      public_key_path + " -signature " +
                      signature_path + " " + file_path;

    std::string result;
    run_cmd(cmd, &result);

    std::remove(public_key_path.c_str());

    return contains_sub_str(result, "Verified OK");
}
