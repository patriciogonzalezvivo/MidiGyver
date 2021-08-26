#pragma once

#include <string>

enum FileType {
    FILE_MIDI
};

class File {
public:

    std::string     name;
    FileType        type;

protected:

    void*           ctx;
};
