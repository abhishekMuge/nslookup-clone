/*

 * CaresResolver.cpp
 *
 *  Created on: 02-May-2025
 *      Author: AbhishekM

*/

#include "CaresResolver.hpp"
#include "Logger.hpp"
#include <ares.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cstdint>
#ifdef _WIN32
  #define _WIN32_WINNT 0x0600
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
  // Define DNS constants for Windows
  #define ns_c_in 1       // IN class
  #define ns_t_cname 5    // CNAME record type
#else
  #include <netdb.h>
  #include <arpa/inet.h>
  #include <arpa/nameser.h>
  #include <sys/select.h>
#endif

struct CaresResolver::Impl {
    ares_channel channel;

    Impl() {
#ifdef _WIN32
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
#endif
        ares_library_init(ARES_LIB_INIT_ALL);

        ares_options options;
        memset(&options, 0, sizeof(ares_options));
        options.flags = ARES_FLAG_NOCHECKRESP;

        int status = ares_init_options(&channel, &options, ARES_OPT_TIMEOUTMS);
        if (status != ARES_SUCCESS) {
            throw std::runtime_error("Failed to initialize c-ares with options: " + std::string(ares_strerror(status)));
        }
    }

    ~Impl() {
        ares_destroy(channel);
        ares_library_cleanup();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    static void callback(void* arg, int status, int /*timeouts*/, struct hostent* host) {
        auto* cb = static_cast<ResultCallback*>(arg);
        std::vector<std::string> results;

        if (status == ARES_SUCCESS && host) {
            for (int i = 0; host->h_addr_list[i]; ++i) {
                char ip[INET6_ADDRSTRLEN];
                const void* addr_ptr = host->h_addr_list[i];
                inet_ntop(host->h_addrtype, addr_ptr, ip, sizeof(ip));
                results.emplace_back(ip);
            }
            (*cb)(results, "");
        } else {
            (*cb)({}, ares_strerror(status));
        }
        delete cb;
    }

    /*static void cnameCallback(void* arg, int status, int timeouts, unsigned char* abuf, int alen) {
        auto* cb = static_cast<ResultCallback*>(arg);
        std::vector<std::string> aliases;

        if (status == ARES_SUCCESS) {
            char cname[1024];  // Buffer to store the alias

            // Parse the CNAME from the response
            if (ares_parse_cname_reply(abuf, alen, nullptr, cname, sizeof(cname)) == ARES_SUCCESS) {
                aliases.emplace_back(cname);
            }
        }

        (*cb)(aliases, status == ARES_SUCCESS ? "" : ares_strerror(status));
        delete cb;
    }*/
    static void cnameCallback(void* arg, int status, int /*timeouts*/, unsigned char* abuf, int alen)
    {
        auto* cb = static_cast<ResultCallback*>(arg);
        std::vector<std::string> aliases;

        if (status == ARES_SUCCESS) {
            // DNS header is 12 bytes, skip it
            int qdcount = (abuf[4] << 8) | abuf[5];  // Number of questions
            int ancount = (abuf[6] << 8) | abuf[7];  // Number of answers

            unsigned char* ptr = abuf + 12;

            // Skip the question section
            for (int i = 0; i < qdcount; ++i) {
                while (*ptr && (ptr < abuf + alen)) {
                    ptr += (*ptr) + 1;
                }
                ptr += 1;  // skip null byte
                ptr += 4;  // skip QTYPE and QCLASS
            }

            // Now in the answer section
            for (int i = 0; i < ancount; ++i) {
                if (ptr >= abuf + alen) break;

                // Skip NAME (can be compressed)
                if ((*ptr & 0xC0) == 0xC0) {
                    ptr += 2;
                } else {
                    while (*ptr && (ptr < abuf + alen)) {
                        ptr += (*ptr) + 1;
                    }
                    ptr += 1;  // null terminator
                }

                if (ptr + 10 > abuf + alen) break;

                uint16_t type = (ptr[0] << 8) | ptr[1];
                ptr += 2;
                ptr += 2;  // class
                ptr += 4;  // TTL
                uint16_t rdlength = (ptr[0] << 8) | ptr[1];
                ptr += 2;

                if (ptr + rdlength > abuf + alen) break;

                if (type == 5 /* CNAME */) {
                    char* cname = nullptr;
                    long expanded_len = 0;
                    int len = ares_expand_name(ptr, abuf, alen, &cname, &expanded_len);
                    if (len >= 0 && cname != nullptr) {
                        aliases.emplace_back(cname);
                        ares_free_string(cname);
                    }
                }

                ptr += rdlength;
            }
        }

        (*cb)(aliases, status == ARES_SUCCESS ? "" : ares_strerror(status));
        delete cb;
    }

    void resolve(const std::string& hostname, int family, ResultCallback callback) {
        auto* cb_copy = new ResultCallback(std::move(callback));
        ares_gethostbyname(channel, hostname.c_str(), family, Impl::callback, cb_copy);
    }

    void resolveCNAME(const std::string& hostname, ResultCallback callback) {
        auto* cb_copy = new ResultCallback(std::move(callback));
        ares_query(channel, hostname.c_str(), ns_c_in, ns_t_cname, Impl::cnameCallback, cb_copy);
    }

    void wait() {
        while (true) {
            fd_set read_fds, write_fds;
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            struct timeval timeout;
            int nfds = ares_fds(channel, &read_fds, &write_fds);
            if (nfds == 0) break;
            Logger::info("Calling ares_timeout...");

            struct timeval* tvp = ares_timeout(channel, nullptr, &timeout);
            if (!tvp) {
                Logger::error("ares_timeout returned null pointer.");
                return;
            }
#ifdef _WIN32
            SOCKET max_fd = 0;
            for (u_int i = 0; i < (u_int)nfds; ++i) {
                if (FD_ISSET(i, &read_fds) || FD_ISSET(i, &write_fds)) {
                    max_fd = std::max(max_fd, (SOCKET)i);
                }
            }
            select(static_cast<int>(max_fd + 1), &read_fds, &write_fds, nullptr, tvp);
#else
            select(nfds, &read_fds, &write_fds, nullptr, tvp);
#endif
            ares_process(channel, &read_fds, &write_fds);
        }
    }
};

CaresResolver::CaresResolver() : impl(new Impl()) {}
CaresResolver::~CaresResolver() { delete impl; }

void CaresResolver::internalResolve(const std::string& hostname, int family, ResultCallback callback) {
    impl->resolve(hostname, family, std::move(callback));
}

void CaresResolver::resolveA(const std::string& hostname, ResultCallback callback) {
    internalResolve(hostname, AF_INET, std::move(callback));
}

void CaresResolver::resolveAAAA(const std::string& hostname, ResultCallback callback) {
    internalResolve(hostname, AF_INET6, std::move(callback));
}

void CaresResolver::resolveCNAME(const std::string& hostname, ResultCallback callback) {
    impl->resolveCNAME(hostname, std::move(callback));
}

void CaresResolver::waitForCompletion() {
    impl->wait();
}


