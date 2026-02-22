#pragma once

#include "common/Types.h"

#include <fstream>
#include <string>

namespace kt {

class CsvWriter {
public:
    bool open(const std::string& path);
    void write(const TimeSeriesRecord& record);
    void flush();
    void close();

private:
    std::ofstream m_file;
    bool          m_headerWritten = false;
};

} // namespace kt
