# RedisLite — A Multithreaded In-Memory Key-Value Store in C

RedisLite is a Redis-inspired, multithreaded in-memory key-value store built from scratch in C.

The project uses raw TCP sockets for client-server communication, a custom thread pool for concurrent request handling, and a hash table as the storage backend. It was built as a systems programming project to better understand networking, multithreading, synchronization, and low-level memory management.

---

## Features

- TCP client-server architecture using POSIX sockets
- Fixed-size thread pool with a producer-consumer queue
- Thread-safe in-memory hash table
- Graceful server shutdown using SIGINT, condition variables, and thread joining.
- Multiple concurrent client connections
- Command parsing and request validation
- Supported commands:
  - CREATE
  - SET
  - GET
  - KEYS
  - DELETE
  - EXISTS
  - COUNT
  - PING

The server currently supports a single shared in-memory database accessed concurrently by multiple clients.

---

## Tech Stack

- C
- POSIX Threads (pthreads)
- Berkeley/POSIX Sockets
- Make
- GDB (debugging)

---

## Architecture

```
┌─────────────┐        TCP         ┌──────────────────────────────────┐
│  tcpclient  │ ◄────────────────► │           tcpserver              │
│  (client)   │     port 8080      │                                  │
└─────────────┘                    │  accept() → pool_submit()        │
                                   │         │                        │
                                   │  ┌──────▼──────┐                 │
                                   │  │  Threadpool │                 │
                                   │  │ (10 threads)│                 │
                                   │  └──────┬──────┘                 │
                                   │         │ worker threads         │
                                   │  ┌──────▼──────┐                 │
                                   │  │ Hash Table  │                 │
                                   │  │ (database)  │                 │
                                   │  └─────────────┘                 │
                                   └──────────────────────────────────┘
```

- **`tcpserver.c`** — Creates a welcome socket, accepts incoming connections, and submits each connection fd to the thread pool. Handles `SIGINT` for graceful shutdown.
- **`tcpclient.c`** — Connects to the server and exposes a simple REPL prompt to send commands.
- **`threadpool.c/h`** — A fixed-size thread pool with a circular queue. Worker threads block on a condition variable until a connection is available.
- **`database.c/h`** — An in-memory hash table using separate chaining (linked lists). Protected by a single mutex (`dblock`) for thread safety.

---

## Prerequisites

- GCC
- POSIX threads (`pthreads`) — available by default on Linux/macOS
- `make`

---

## Building

Clone the repo and run:

```bash
git clone https://github.com/sarvaniun/RedisLite.git
cd RedisLite
make
```

This produces two binaries: `server` and `client`.

To clean up:

```bash
make clean
```

---

## Running

Start the server:

```bash
./server
```

In another terminal:

```bash
./client
```

Open multiple client terminals to interact with the server concurrently.

## Graceful Shutdown

Pressing `Ctrl+C` on the server initiates a graceful shutdown.

The server:

- Stops accepting new client connections.
- Allows existing client sessions to finish.
- Wakes sleeping worker threads.
- Joins all worker threads.
- Cleans up allocated resources before exiting.

---
## Commands

All commands are case-sensitive. Send them from the client prompt.

| Command | Format | Description |
|---|---|---|
| `CREATE` | `CREATE` | Initializes the in-memory hash table. Must be called first. |
| `PING` | `PING` | Health check. Returns `PONG`. |
| `SET` | `SET <key> <value>` | Inserts or updates a key-value pair. |
| `GET` | `GET <key>` | Returns the value for a key, or `NULL` if not found. |
| `DELETE` | `DELETE <key>` | Removes a key. Returns `ERR Key not found` if missing. |
| `EXISTS` | `EXISTS <key>` | Returns `YES` or `NO`. |
| `COUNT` | `COUNT` | Returns the total number of keys stored. |
| `KEYS` | `KEYS` | Lists all keys currently in the table. |

### Example Session

```
> CREATE
OK
> SET name Alice
OK
> SET city Bangalore
OK
> GET name
Alice
> EXISTS city
YES
> COUNT
2
> KEYS
name
city
> DELETE name
OK
> GET name
NULL
> COUNT
1
> PING
PONG
```

---

## Implementation Notes

### Hash Table

The hash table uses **26 buckets** (`MAX_BUCKETS`) with **separate chaining** to resolve collisions. The hash function is the djb2 algorithm — a simple multiplicative hash (`hash * 33 + c`) that distributes keys well in practice.

Keys and values are fixed-size char arrays (`MAX_CHARS = 100`). The table is protected by a single `pthread_mutex_t` - all reads and writes acquire the same lock `dblock`.

### Thread Pool

The thread pool maintains a fixed array of `pthread_t` threads and a **circular queue** of connection socket file descriptors. Two condition variables coordinate producers and consumers:

- `task_available` — workers wait on this when the queue is empty.
- `space_available` — the main thread waits on this when the queue is full (`QUEUE_SIZE = 256`).

---

## Future Improvements

- RESP protocol compatibility
- Persistent storage
- Dynamic hash table resizing
- Per-bucket locking for higher concurrency
- Client `QUIT` command
- Configuration file support
- Benchmarking tools

---

## File Structure

```
.
├── Makefile
├── tcpserver.c      # Server: socket setup, accept loop, signal handling
├── tcpclient.c      # Client: REPL prompt over TCP
├── threadpool.c     # Thread pool: worker logic, queue, create/submit/destroy
├── threadpool.h
├── database.c       # Hash table: get, set, delete, exists, count, keys
└── database.h
```

## What I Learned

Through this project, I gained more hands-on experience with:

- TCP socket programming
- POSIX threads
- Producer-consumer synchronization
- Mutexes and condition variables
- Concurrent programming
- GDB debugging
- Hash table implementation
- Low-level memory management in C

---