// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include <iostream>
#include "options_parser.h"
#include <fcntl.h>
#include <filesystem>

int writebuffer(int fd, const char* buffer, ssize_t size) {
    ssize_t written_bytes = 0;

    while (written_bytes < size) {
        ssize_t written_now = write(fd, buffer + written_bytes, size - written_bytes);

        if (written_now == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("Error on write");
                return -1;
            }
        } else {
            written_bytes += written_now;
        }
    }
    return 0;
}

int readbuffer(int fd, char* buffer, ssize_t size) {
    ssize_t read_bytes = 0;

    while (read_bytes < size) {
        ssize_t written_now = read(fd, buffer + read_bytes, size - read_bytes);

        if (written_now == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("Error on read");
                return -1;
            }
        }
        else if (written_now == 0){
            writebuffer(1, buffer, read_bytes);
            return 1;
        } else {
            read_bytes += written_now;
        }
    }
    return 0;
}

int transform_buffer(char* buffer, ssize_t buf_size, char* temp_buffer, ssize_t temp_buf_size) {
    int tmp_size = 0;

    for (ssize_t i = 0; i < buf_size; i++) {
        char ch = buffer[i];

        if (!isspace(ch) && !isprint(ch)) {
            if (tmp_size + 4 <= temp_buf_size) {
                snprintf(temp_buffer + tmp_size, 5, "\\x%02X", static_cast<unsigned char>(ch));
                tmp_size += 4;
            } else {
                perror("Error on transforming buffer");
                exit(1);
            }
        } else {
            if (tmp_size + 1 <= temp_buf_size) {
                temp_buffer[tmp_size++] = ch;
            } else {
                perror("Error on transforming buffer");
                exit(1);
            }
        }
    }
    return tmp_size;
}

void assert_file_exist(const std::string &f_name) {
    std::error_code ec;
    if (!std::filesystem::exists(f_name)) {
        perror("Nonexistent file passed");
        exit(1);
    }
    if (!std::filesystem::is_regular_file(f_name, ec)){
        perror("Passed file is not a file");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    const ssize_t buf_size = 512;
    char buf[buf_size];
    char temp_buff[buf_size*4];
    command_line_options_t command_line_options{argc, argv};
    // std::cout << "A flag value: " << command_line_options.get_A_flag() << std::endl;
    for (const std::string& file : command_line_options.get_filenames()) {
        assert_file_exist(file);}
    for (const std::string& file : command_line_options.get_filenames()) {
        int fd = open(file.c_str(), O_RDONLY);
        if (fd<0){
            perror("Error while opening file");
            exit(1);
        }
        
        if (!command_line_options.get_A_flag()){
            while (readbuffer(fd, buf, buf_size)==0){
                writebuffer(1, buf, buf_size);
            }
        } else{
            while (readbuffer(fd, buf, buf_size)==0){
                auto temp_size = transform_buffer(buf, buf_size, temp_buff, buf_size*4);
                writebuffer(1, temp_buff, temp_size);
            }
        }
        // write(1, "\n", 2);
        close(fd);
    }
    return 0;
}

