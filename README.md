# 💬 IPC-Chat

A lightweight, terminal-based **chat application written in C**, designed to demonstrate **low-level inter-process and network communication**.  
Ships as a single portable executable: `s-talk`.

![C](https://img.shields.io/badge/language-C-blue.svg)
![Makefile](https://img.shields.io/badge/build-Makefile-brightgreen.svg)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)
![License](https://img.shields.io/badge/license-MIT-orange.svg)

---

## 🧠 Overview

`IPC-Chat` is a small yet powerful program that showcases **systems-level programming**, combining:
- **Sockets and TCP networking**
- **Non-blocking I/O using `select()`**
- **Signal handling and process synchronization**
- **Lightweight command-line UX**

The entire project compiles into one binary, making it ideal for studying **UNIX communication primitives** or showcasing **C networking expertise** during interviews.

---

## 🚀 Key Highlights
- Implemented a **two-way chat system** in pure C with **manual socket management** and **graceful connection handling**.
- Demonstrates **real-time communication** using `select()` and **multi-process synchronization**.
- Fully portable — no external dependencies, compiled via **GNU Make**.
- Serves as a **concise educational reference** for inter-process communication and socket programming.

---

## ⚙️ Tech Stack
| Category | Technologies |
|-----------|--------------|
| **Language** | C |
| **Build System** | GNU Make |
| **Key Concepts** | Sockets, TCP Connections, Non-blocking I/O, `select()`/`poll()`, Process Signaling, CLI UX |

---

## 🧩 Quick Start

1. **Build**  
   ```bash
   make
   ```

Produces the `s-talk` executable.

2. **Run a Listener** (on machine A)  
   Replace `12345` with any open port:  
   ```bash
   ./s-talk 12345
   ```

3. **Connect from Another Machine (or Terminal)** by specifying your local port (or `0` to auto-bind), the remote host, and remote port:  
   ```bash
   ./s-talk 0 example.com 12345
   ```

**Notes:**
- If you run both ends locally for testing, use `localhost` for the remote host and two different terminals.
- To exit: press `!` then Enter (this requests a coordinated exit; if one side disconnects unexpectedly, terminate with Ctrl+C).

## 🖥️ Local Test Example

- Terminal 1:  
  ```bash
  ./s-talk 12345
  ```
- Terminal 2:  
  ```bash
    ./s-talk 0 localhost 12345
    ```

## 🌐 Remote Test Example

- On remote host
    ```bash
    ./s-talk 23456
    ```

- On local machine
    ```bash
    ./s-talk 0 remote-host 23456
    ```

To exit: press `!` then Enter (this requests a coordinated exit; if one side disconnects unexpectedly, terminate with Ctrl+C).


## 🎥 Demo & Assets




## 🧠 Notes for Interviewers

The code is intentionally minimal and educational. Focus areas for review:
- socket setup and teardown
- message loop and non-blocking I/O using `select()`
- error handling and resource cleanup