# 🧠 Project Title: Advanced DNS Lookup Tool (C++ with c-ares)

## 📋 Description

This project is a **feature-rich DNS lookup utility** implemented in **C++** using the **c-ares asynchronous DNS library**. It replicates and enhances the functionality of the traditional `nslookup` tool, while introducing modern capabilities and flexible options.

The tool supports domain resolution for **A (IPv4)**, **AAAA (IPv6)**, and **CNAME** records, allows the use of **custom DNS servers**, and enables **multiple domain lookups** in a single run with clean, readable output.

---

## ✨ Key Features

- ✅ **Asynchronous DNS resolution** using `c-ares` for **non-blocking performance**.
- 🌐 **Supports multiple domain arguments** via command-line input.
- 📡 **Custom DNS server** support (e.g., `8.8.8.8` or `1.1.1.1`).
- 🔁 **CNAME alias parsing**: Extracts and displays alias chains from DNS responses.
- 📄 **Clean output** similar to `nslookup`, showing both IPv4 and IPv6 addresses along with any aliases.
- 🛠️ **Regex-based preprocessing**: Automatically strips `http://` or `https://` from input domains.
- 🧵 Built with **multithreading and event-driven design** internally via `c-ares` for maximum efficiency.

---

## 🚀 Example Usage

```bash
./dnslookup example.com www.test.com --dns 8.8.8.8
