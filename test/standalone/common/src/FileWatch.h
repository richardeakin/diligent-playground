#pragma once

#include <filesystem>

// https://github.com/ThomasMonkman/filewatch
#include "FileWatch-Monkman.hpp"

namespace ju {

// TODO: try fs::path again and see if the path comes through correctly, I was thrown off by the temporary files
//using PathType = std::filesystem::path; // FIXME: can't use std::filesystem until this is addressed: https://github.com/ThomasMonkman/filewatch/issues/39
using PathType = std::string;
using FileWatchType = filewatch::FileWatch<PathType>;
using FileWatchHandle = std::unique_ptr<FileWatchType>;

inline const char* watchEventTypeToString( const filewatch::Event change_type )
{
    switch (change_type) {
        case filewatch::Event::added:			return "added";
        case filewatch::Event::removed:			return "removed";
        case filewatch::Event::modified:		return "modified";
        case filewatch::Event::renamed_old:		return "renamed_old";
        case filewatch::Event::renamed_new:		return "renamed_new";
        default: break;
    };

    return "(unknown)";
}

} // ju
