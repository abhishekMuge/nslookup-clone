/*
 * Logger.cpp
 *
 *  Created on: 02-May-2025
 *      Author: AbhishekM
 */

#include "Logger.hpp"
#include <iostream>

void Logger::info(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}



