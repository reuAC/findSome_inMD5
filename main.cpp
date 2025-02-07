#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <fstream>
#include <random>
#include <iomanip>
#include <sstream>
#include <openssl/evp.h>
#include <future>

using namespace std;


string target_substring = "114514";
string output_filename = "c.txt";
bool running = true;
mutex file_mutex;

string generate_random_string(int max_length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static random_device rd;
    static mt19937 generator(rd());
    static uniform_int_distribution<int> length_distribution(1, max_length);
    static uniform_int_distribution<int> char_distribution(0, sizeof(charset) - 2);

    int random_length = length_distribution(generator);
    string random_str = "";
    for (int i = 0; i < random_length; ++i) {
        random_str += charset[char_distribution(generator)];
    }
    return random_str;
}

string calculate_md5(const string& str) {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    if (!mdctx) {
        return "";
    }
    const EVP_MD* md = EVP_md5();
    if (!md) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    if (EVP_DigestInit_ex(mdctx, md, NULL) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    if (EVP_DigestUpdate(mdctx, str.c_str(), str.length()) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len;
    if (EVP_DigestFinal_ex(mdctx, digest, &digest_len) != 1) {
        EVP_MD_CTX_free(mdctx);
        return "";
    }
    EVP_MD_CTX_free(mdctx);

    stringstream ss;
    for (int i = 0; i < digest_len; ++i) {
        ss << hex << setw(2) << setfill('0') << (int)digest[i];
    }
    return ss.str();
}

void async_write_to_file(const string& str_to_write) {
    lock_guard<mutex> lock(file_mutex);
    ofstream output_file(output_filename, ios::app);
    if (output_file.is_open()) {
        output_file << str_to_write << endl;
        output_file.close();
    }
    else {
        cerr << "无法打开文件 " << output_filename << " 进行写入!" << endl;
    }
}

void worker_thread_function() {
    while (running) {
        string random_str = generate_random_string(32);
        string md5_value = calculate_md5(random_str);

        if (md5_value.find(target_substring) != string::npos) {
            string output_string = "找到匹配字符串: " + random_str + ", MD5: " + md5_value;

            cout << output_string << endl;

            future<void> write_future = async(launch::async, async_write_to_file, "字符串: " + random_str + ", MD5: " + md5_value);
        }
    }
}


int main() {
    int num_threads = thread::hardware_concurrency();
    if (num_threads == 0) {
        num_threads = 4;
    }
    cout << "使用 " << num_threads << " 个线程进行计算..." << endl;

    vector<thread> worker_threads;
    for (int i = 0; i < num_threads; ++i) {
        worker_threads.emplace_back(worker_thread_function);
    }

    cout << "程序正在运行，按 Ctrl+C 停止..." << endl;

    cin.get();
    running = false;

    for (auto& t : worker_threads) {
        t.join();
    }

    cout << "程序已停止。" << endl;

    return 0;
}