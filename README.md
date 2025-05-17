MTTypeTestingGame

A real-time terminal-based typing-speed game in C. This project features a multithreaded POSIX-sockets server handling multiple clients concurrently, and a raw-mode client capturing keystrokes for live WPM and accuracy scoring, complete with anti-cheat detection and leaderboard support.

Features

Multithreaded server: Each client runs in its own detached pthread, allowing dozens of players to practice simultaneously.

POSIX sockets: TCP-based client-server communication on configurable ports.

Typing test mode:

100-word random passages generated server-side.

15-second timed tests with real-time keystroke streaming.

WPM and accuracy calculation, with paste/copy detection.

Chat & commands:

Private messaging (/msg), broadcast (/broadcast).

User list (/list), high-score retrieval (/highscore), and graceful quit (/quit).

Anti-cheat: Detects paste-based input bursts and invalid timing patterns.

Leaderboard persistence: Stores and updates the top WPM record with timestamps in a text file.

Prerequisites

C compiler (GCC or Clang)

POSIX-compliant system (Linux, macOS)

pthread and socket libraries (standard on most Unix-like OS).

Building

Clone the repository:

git clone https://github.com/ShaheemJ/MTTypeTestingGame.git
cd MTTypeTestingGame

Compile with gcc:

gcc -Wall -Wextra -pedantic -pthread typetestingserver.c -o typetestingserver
gcc -Wall -Wextra -pedantic typetestingclient.c -o typetestingclient

(Optional) Use provided CMake setup:

mkdir build && cd build
cmake ..
make

Usage

Start the server (default port 5000+):

./typetestingserver

The server binds to the first available port starting at 4999.

Connect a client:

./typetestingclient <server_ip> <port>

Example:

./typetestingclient 127.0.0.1 5000

Commands:

/play — Start a 15-second typing test.

/msg <user> <text> — Send a private message.

/broadcast <text> — Broadcast to all users.

/list — Show connected users.

/highscore — Display the all-time top WPM record.

/quit — Exit.
