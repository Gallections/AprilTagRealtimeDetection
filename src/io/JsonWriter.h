#pragma once

#include "common/Types.h"

#include <fstream>
#include <string>

namespace kt {

/// Newline-delimited JSON (NDJSON) writer for streaming compatibility.
class JsonWriter {
public:
    bool open(const std::string& path);
    void write(const TimeSeriesRecord& record);
    void flush();
    void close();

private:
    std::ofstream m_file;
};

} // namespace kt
