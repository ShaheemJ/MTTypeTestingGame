# MTTypeTestingGame

A real-time terminal-based typing-speed game in C. This project features a **multithreaded** POSIX-sockets server handling multiple clients concurrently, and a **raw-mode** client capturing keystrokes for live WPM and accuracy scoring, complete with anti-cheat detection and leaderboard support.

---

## Features

- **Multithreaded server**  
  Each client runs in its own detached `pthread`, allowing dozens of players to practice simultaneously.  
- **POSIX sockets**  
  TCP-based client-server communication on configurable ports.  
- **Typing test mode**  
  - 100-word random passages generated server-side  
  - 15-second timed tests with real-time keystroke streaming  
  - WPM & accuracy calculation, with paste/copy detection  
- **Chat & commands**  
  - Private messaging: `/msg <user> <text>`  
  - Broadcast: `/broadcast <text>`  
  - User list: `/list`  
  - High-score: `/highscore`  
  - Quit: `/quit`  
- **Anti-cheat**  
  Detects paste-based input bursts and impossible typing speeds.  
- **Leaderboard persistence**  
  Stores and updates the top WPM record (with timestamp) in a text file.

---

## Prerequisites

- **C compiler** (GCC or Clang)  
- **POSIX-compliant system** (Linux, macOS)  
- **pthread** and **socket** libraries (standard on most Unix-like OS)

---

## Building

```bash
# Clone the repo and enter the directory
git clone https://github.com/ShaheemJ/MTTypeTestingGame.git
cd MTTypeTestingGame

# Compile the server
gcc -Wall -Wextra -pedantic -pthread typetestingserver.c -o typetestingserver

# Compile the client
gcc -Wall -Wextra -pedantic typetestingclient.c -o typetestingclient

