//============================================================================
// Name        : Advance.cpp
// Author      : AbhishekM
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================
//#include "DNSLookup.hpp"
#include "CaresResolver.hpp"
//#include "Exporter.hpp"
#include "Logger.hpp"
#include <iostream>
#include <regex>

using namespace std;


std::string removeHttpPrefix(const std::string& url) {
    std::regex regex("^(https?://)");
    return std::regex_replace(url, regex, "");
}


int main(int argc, char** argv) {
    if (argc < 2) {
        Logger::error("Usage: ./dnslookup [...<domain>] ");
        return 1;
    }

    /*std::string domain = argv[1];
    std::string dnsServer = argc > 2 ? argv[2] : "";*/

    std::vector<std::string> domains;
    std::string dnsServer = /*argc > 1 ? argv[argc - 2] :*/ "";  // Optional DNS server (second to last argument)

    for (int i = 1; i < argc; ++i)
    {  // Exclude DNS server and export format if provided
    	domains.emplace_back(removeHttpPrefix(argv[i]));
    }

    try {
        CaresResolver resolver;
        std::vector<std::string> ipv4Results, ipv6Results, cnameResults;
        bool aDone = false, aaaaDone = false, cnameDone = false;

        for(const auto &domain: domains)
        {
        	resolver.resolveA(domain, [&](const std::vector<std::string>& ips, const std::string& error) {
				if (!error.empty()) {
					Logger::error("[IPv4] DNS Lookup failed: " + error);
				} else {
					ipv4Results = ips;
				}
				aDone = true;
			});

			resolver.resolveAAAA(domain, [&](const std::vector<std::string>& ips, const std::string& error) {
				if (!error.empty()) {
					Logger::error("[IPv6] DNS Lookup failed: " + error);
				} else {
					ipv6Results = ips;
				}
				aaaaDone = true;
			});

			resolver.resolveCNAME(domain, [&](const std::vector<std::string>& aliases, const std::string& error) {
				if (!error.empty()) {
					Logger::error("[CNAME] Alias Lookup failed: " + error);
				} else {
					cnameResults = aliases;
				}
				cnameDone = true;
			});

			resolver.waitForCompletion();

			// Output in a format similar to nslookup
			std::cout << "\nServer:\t\t" << (dnsServer.empty() ? "default" : dnsServer) << "\n";
			std::cout << "Address:\t" << (dnsServer.empty() ? "127.0.0.1" : dnsServer) << "\n\n"; // Placeholder

			std::cout << "Non-authoritative answer:\n";
			std::cout << "Name:\t\t" << domain << "\n";

			for (const auto& cname : cnameResults) {
				std::cout << "Aliases:\t" << cname << "\n";
			}

			for (const auto& ip : ipv4Results) {
				std::cout << "Address:\t" << ip << "\n";
			}

			for (const auto& ip6 : ipv6Results) {
				std::cout << "Address:\t" << ip6 << "\n";
			}
			std::cout << "*************************************************\n";
        }

    } catch (const std::exception& e) {
        Logger::error(std::string("Error: ") + e.what());
        return 1;
    }

    return 0;
}
