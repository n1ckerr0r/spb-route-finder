#include <iostream>
#include <stdint.h>
#include <cstdlib>
#include <vector>

#ifndef CMD_PARSER_H
#define CMD_PARSER_H

class CmdParser {
public:
    CmdParser(uint32_t argc, char* argv[]);
    void furtherParsing(std::string& data);
    bool dataProcessing(std::string& data);
    void Help();
    bool isValidData(std::vector<uint16_t>& data);
    bool isDataInRange(std::vector<uint16_t>& data);
    void createFormatData(std::vector<uint16_t> &data);
private:
    std::string format_data;
};

#endif
