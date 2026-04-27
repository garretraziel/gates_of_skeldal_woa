#include "../save_folder.h"
#include "../error.h"
#include <filesystem>

static std::string get_default_savegame_dir() {

    char *home = std::getenv("HOME");
    if (home) {
        return std::filesystem::path(home) / "Library/Application Support" / SAVEGAME_FOLDERNAME;
    } else {
        display_error("$HOME has no value (user with no home)");
        abort();
    }
}




const char *get_default_savegame_directory() {

    static std::string dir = get_default_savegame_dir();
    return dir.c_str();

}
