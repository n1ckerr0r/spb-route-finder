#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <iomanip>
#include <algorithm>
#include <chrono>

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

class CmdParser {
public:
    CmdParser(uint32_t argc, char* argv[], std::string& finish_data);
    void furtherParsing(std::string& data);
    bool dataProcessing(std::string& data);
    void Help();
    bool isValidData(std::vector<uint16_t>& data);
    bool isDataInRange(std::vector<uint16_t>& data);
    void createFormatData(std::vector<uint16_t> &data);

    void createFormatData(const std::vector<unsigned short> & data, const char * str);
private:
    std::string format_data;
};

#endif
