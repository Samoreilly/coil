

#include <vector>
#include <string>


class FileHandler {

    std::vector<std::string> file_names;

public:

    FileHandler(char* argv[], int size) {
        (void)argv;
        (void)size;
    }

    
    std::string get_file_content(std::string file_name);
    std::vector<std::string> load_files(char* argv[], int size);
    std::vector<std::string> get_folder_content(std::string folder_name);


};
