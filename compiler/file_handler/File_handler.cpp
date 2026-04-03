
#include "File_handler.h"
#include <fmt/core.h>
#include <vector>
#include <string>
#include <fstream>
#include <filesystem>




std::vector<std::string> FileHandler::load_files(char* argv[], int size) {
    
    if(size == 2 && std::filesystem::is_directory(argv[1])) {
        return get_folder_content(std::string(argv[1]));
    }


    std::vector<std::string> file_contents;
    
    for(int i = 1;i < size;i++) {
        char* file_name = argv[i];
        fmt::println("load_files() {}", file_name);
        file_contents.push_back(get_file_content(std::string(file_name)));
    }

    return file_contents;

}

std::vector<std::string> FileHandler::get_folder_content(std::string dir) {

    std::vector<std::string> file_contents;
    std::filesystem::path curr; 
    
    if(dir.empty()) {
        curr = std::filesystem::current_path();
    }else {
        curr = std::filesystem::path(dir);
    }

    fmt::println("Test1");
    fmt::print(stderr, "Current dir {}", curr.string());
    
    for(const auto& f : std::filesystem::directory_iterator(curr)) {
        
        if(f.is_regular_file() && f.path().extension() == ".coil") {
            file_contents.push_back(get_file_content(f.path().string()));
        }
    }

    return file_contents;
}


std::string FileHandler::get_file_content(std::string file_name) {

    std::ifstream file(file_name);

    if(!file.is_open()) {
        throw std::runtime_error("File " + file_name + " was not found");
    }

    std::string line, file_content;
    while(std::getline(file, line)) {
        file_content += (line + "\n");
    }
    
    file.close();

    return file_content;
}
