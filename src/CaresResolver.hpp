/*
 * CaresResolver.hpp
 *
 *  Created on: 02-May-2025
 *      Author: AbhishekM
 */

#ifndef CARESRESOLVER_HPP_
#define CARESRESOLVER_HPP_


#pragma once
#include <string>
#include <vector>
#include <functional>

class CaresResolver {
public:
    using ResultCallback = std::function<void(const std::vector<std::string>&, const std::string& error)>;

    CaresResolver();
    ~CaresResolver();

    void resolveA(const std::string& hostname, ResultCallback callback);
    void resolveAAAA(const std::string& hostname, ResultCallback callback);
    void resolveCNAME(const std::string& hostname, ResultCallback callback);
    void waitForCompletion();

private:
    void internalResolve(const std::string& hostname, int family, ResultCallback callback);
    struct Impl;
    Impl* impl;
};



#endif /* CARESRESOLVER_HPP_ */
