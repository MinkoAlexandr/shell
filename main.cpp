#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <signal.h>
#include <cstdint>

#include "vfs.hpp"

void handle_sighup(int sig_num) {
    if (sig_num == SIGHUP) {
        std::cout << "Configuration reloaded\n";
        std::cout << "$ ";
    }
}

void scan_disk(const std::string& disk_path) {
    std::ifstream disk_file(disk_path, std::ios::binary);
    
    if (!disk_file) {
        std::cout << "Failed to access " << disk_path << "\n";
        return;
    }
    
    unsigned char boot_sector[512];
    disk_file.read((char*)boot_sector, 512);
    
    if (disk_file.gcount() != 512) {
        std::cout << "Disk read failure\n";
        return;
    }
    
    if (boot_sector[510] != 0x55 || boot_sector[511] != 0xAA) {
        std::cout << "Invalid disk marker\n";
        return;
    }
    
    bool is_gpt_format = false;
    for (int part_idx = 0; part_idx < 4; part_idx++) {
        int table_start = 446 + part_idx * 16;
        if (boot_sector[table_start + 4] == 0xEE) {
            is_gpt_format = true;
            break;
        }
    }
    
    if (!is_gpt_format) {
        for (int part_idx = 0; part_idx < 4; part_idx++) {
            int table_offset = 446 + part_idx * 16;
            unsigned char partition_type = boot_sector[table_offset + 4];
            
            if (partition_type != 0) {
                uint32_t sector_count = *(uint32_t*)&boot_sector[table_offset + 12];
                uint32_t size_mb = sector_count / 2048;
                bool can_boot = (boot_sector[table_offset] == 0x80);
                
                std::cout << "Part " << (part_idx + 1) << ": " 
                          << size_mb << "MB, Boot: " 
                          << (can_boot ? "Yes" : "No") << "\n";
            }
        }
    } else {
        disk_file.read((char*)boot_sector, 512);
        if (disk_file.gcount() == 512 && 
            boot_sector[0] == 'E' && boot_sector[1] == 'F' && 
            boot_sector[2] == 'I' && boot_sector[3] == ' ' &&
            boot_sector[4] == 'P' && boot_sector[5] == 'A' &&
            boot_sector[6] == 'R' && boot_sector[7] == 'T') {
            
            uint32_t part_count = *(uint32_t*)&boot_sector[80];
            std::cout << "GPT partitions found: " << part_count << "\n";
        } else {
            std::cout << "GPT data unavailable\n";
        }
    }
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    init_virtual_fs();

    const char* user_home = std::getenv("HOME");
    std::string history_file = std::string(user_home) + "/.kubsh_log";

    signal(SIGHUP, handle_sighup);
    
    std::cout << "$ ";

    for (std::string cmd_line; std::getline(std::cin, cmd_line);) {
        if (!cmd_line.empty()) {
            std::ofstream log_stream(history_file, std::ios::app);
            log_stream << cmd_line << "\n";
        }

        if (cmd_line == "history") {
            std::ifstream history_stream(history_file);
            std::string previous_cmd;
            while (std::getline(history_stream, previous_cmd)) {
                std::cout << previous_cmd << "\n";
            }
        } else if (cmd_line == "\\q") {
            break;
        } else if (cmd_line.substr(0, 3) == "\\l ") {
            std::string disk_name = cmd_line.substr(3);
            disk_name.erase(0, disk_name.find_first_not_of(" \t"));
            disk_name.erase(disk_name.find_last_not_of(" \t") + 1);
            
            if (disk_name.empty()) {
                std::cout << "Format: \\l /dev/disk_name\n";
            } else {
                scan_disk(disk_name);
            }
        } else if (cmd_line.substr(0, 7) == "debug '" && cmd_line[cmd_line.length() - 1] == '\'')
        {

            std::cout << cmd_line.substr(7, cmd_line.length() - 8) << std::endl;  
            continue;  

        }
          else if (cmd_line.substr(0,4) == "\\e $")
        {
            std::string varName = cmd_line.substr(4);
            const char* value = std::getenv(varName.c_str());//Преобразуем C-строку в C++ строку

            if(value != nullptr)
            {
                std::string valueStr = value;
                
                bool has_colon = false;//Флаг для проверки наличия двоеточий
                for (char c : valueStr)//Проходим по символам из строки
                {
                    if (c == ':') 
                    {
                        has_colon = true;
                        break;
                    }
                }
                
                if (has_colon) 
                {
                    std::string current_part = "";//Временная строка для накопления текущей части пути
                    for (char c : valueStr)//Разбиваем строку по двоеточиям

                    {
                        if (c == ':') 
                        {
                            std::cout << current_part << "\n";//Когда встречаем двоеточие - выводим накопленную часть
                            current_part = "";//Сбрасываем временную строку для следующей части
                        }
                        else 
                        {
                            current_part += c;//Иначе добавляем символ к строке

                        }
                    }
                    std::cout << current_part << "\n";//Выводим последнюю часть (после последнего двоеточия)
                }
                else 
                { 
                    std::cout << valueStr << "\n";//Если двоеточий нет - просто выводим значение как есть
                }
            }
            else
            {
                std::cout << varName << ": не найдено\n";
            }
            continue;
        }

    else 
    {
        //Создаем процесс и записываем process id
        pid_t pid = fork();
        
        //Если дочерний процесс(в нем выполним бинарник)
        if (pid == 0) 
        {
            // Создаем копии строк для аргументов
            std::vector<std::string> tokens;
            //Указатели для execvp
            std::vector<char*> args;
            std::string token;
            //Разбиваем по пробелам для аргументов
            std::istringstream iss(cmd_line);
            
            while (iss >> token) 
            {
                tokens.push_back(token);  // Сохраняем копии
            }
            
            // Преобразуем в char*
            for (auto& t : tokens) 
            {
                args.push_back(const_cast<char*>(t.c_str()));
            }
            //Для execvp чтобы видел конец
            args.push_back(nullptr);
            
            //Заменяем программу на новую
            //args[0] - название команды, args.data() - ссылка на C массив строк для аргументов
            execvp(args[0], args.data());
            
            //Если не нашли команду то выведет это(вернет управление), при успехе не дойдем до этих строк
            std::cout << args[0] << ": command not found\n";
            exit(1);
            
        } 
        else if (pid > 0) 
        {
            int status;
            //Ожидаем дочерний
            waitpid(pid, &status, 0);
        } 
        else 
        {
            std::cerr << "Failed to create process\n";
        }
    }
        std::cout<<"$ ";
    }
}
