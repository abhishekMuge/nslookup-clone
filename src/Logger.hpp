/*
 * Logger.hpp
 *
 *  Created on: 02-May-2025
 *      Author: Akshay
 */

#ifndef LOGGER_HPP_
#define LOGGER_HPP_


#pragma once
#include <string>

class Logger {
public:
    static void info(const std::string& msg);
    static void error(const std::string& msg);
};



#endif /* LOGGER_HPP_ */
