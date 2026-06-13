# 🏦 Bank Management System

A fully functional, multi-client **Bank Management System** built in **C** using **Unix socket programming**, **multi-process concurrency**, **POSIX named semaphores**, and **file-level locking** — all running over a TCP client-server architecture.

---

## 📌 Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Project Structure](#project-structure)
- [Data Structures](#data-structures)
- [Roles & Features](#roles--features)
- [Key Technical Concepts](#key-technical-concepts)
- [Workflow](#workflow)
- [How to Build & Run](#how-to-build--run)
- [Use Cases](#use-cases)
- [Limitations & Future Scope](#limitations--future-scope)

---

## Overview

This project simulates a real-world banking system that supports **four types of users** — Customer, Employee, Manager, and Admin — each with their own authenticated session and role-specific operations.

The system is built entirely in **C (Linux/Unix)** and communicates via **TCP sockets** on `localhost:8081`. The server handles multiple simultaneous client connections using `fork()`, ensuring process isolation per session. Concurrent file access is controlled with `fcntl()` record-level locks, and duplicate logins are blocked using **POSIX named semaphores**.

---

## Architecture

```
  ┌───────────────────────┐         ┌────────────────────────────────────────┐
  │      CLIENT           │  TCP    │             SERVER                     │
  │  (client.c)           │◄──────►│         (server.c)                     │
  │                       │ :8081   │                                        │
  │  - Send user input    │         │  fork() per connection                 │
  │  - Display prompts    │         │  ├── customerMenu()                    │
  │  - Hide passwords     │         │  ├── employeeMenu()                    │
  │  - Handle '^' signals │         │  ├── managerMenu()                     │
  └───────────────────────┘         │  └── adminMenu()                       │
                                    │                                        │
                                    │  File I/O (binary flat files)          │
                                    │  ├── customers.txt                     │
                                    │  ├── employees.txt                     │
                                    │  ├── loanDetails.txt                   │
                                    │  ├── trans_hist.txt                    │
                                    │  ├── feedback.txt                      │
                                    │  └── Counter.txt                       │
                                    └────────────────────────────────────────┘
```

---

## Project Structure

```
Software-system-MINI-PROJECT-main/
│
├── ClientAndServer/
│   ├── server.c            # TCP server: accept, fork, dispatch to role handlers
│   └── client.c            # TCP client: relay I/O, hide passwords, handle signals
│
├── Modules/
│   ├── Customer.h          # All customer operations (deposit, withdraw, transfer, etc.)
│   ├── Employee.h          # Employee operations (add customer, loan approval, etc.)
│   ├── Manager.h           # Manager operations (account control, loan assignment, feedback)
│   └── Admin.h             # Admin operations (add employee, modify records, manage roles)
│
├── AllStructures/
│   └── allstructure.h      # Shared struct definitions for all entities
│
├── Utility/
│   └── FetchNextID.h       # Thread-safe auto-increment ID generator
│
├── Data/
│   ├── customers.txt       # Binary customer records
│   ├── employees.txt       # Binary employee/manager records
│   ├── loanDetails.txt     # Binary loan records
│   ├── trans_hist.txt      # Binary transaction history
│   ├── feedback.txt        # Binary customer feedback records
│   └── Counter.txt         # Plain-text ID counters (EMPLOYEE, CUSTOMER, LOANID)
│
├── debug/
│   └── debug.c             # Standalone debug/testing utility
│
└── Diagram.png             # System architecture diagram
```

---

## Data Structures

All records are stored as **binary structs** directly written to files using `write()` / `read()` system calls.

```c
// Customer account record
struct Customer {
    int   accountNumber;    // Auto-assigned (starts from 1000)
    float balance;
    char  firstName[20];
    char  lastName[20];
    char  password[256];
    int   activeStatus;     // 0 = deactivated, 1 = active
};

// Employee / Manager record (distinguished by role)
struct Employee {
    int  empID;             // Auto-assigned (starts from 1)
    char firstName[20];
    char lastName[20];
    char password[256];
    int  role;              // 0 = Manager, 1 = Employee
};

// Loan application record
struct LoanDetails {
    int loanID;             // Auto-assigned (starts from 100)
    int accountNumber;      // Customer requesting the loan
    int empID;              // -1 if unassigned
    int loanAmount;
    int status;             // 0=Requested, 1=Pending, 2=Approved, 3=Rejected
};

// Transaction history entry
struct trans_histroy {
    int  acc_no;
    char hist[1024];        // Timestamped description string
};

// Customer feedback
struct FeedBack {
    char firstName[20];
    char lastName[20];
    char feedback[1024];
};

// Global ID counter
struct Counter {
    int count;
};
```

---

## Roles & Features

### 👤 Customer
Authenticated with **Account Number + Password**.

| Feature | Description |
|---|---|
| Deposit | Add funds to account; record timestamped transaction |
| Withdraw | Deduct funds; checks for sufficient balance |
| View Balance | Fetch real-time balance from file |
| Apply for Loan | Submit loan request; auto-assign Loan ID |
| Money Transfer | Transfer funds between two accounts with dual-record lock |
| Change Password | Update password with file lock; auto-logout after |
| Transaction History | View last 10 transactions |
| Add Feedback | Submit feedback by name linked to account number |
| Logout | Release semaphore, return to main menu |

### 👷 Employee
Authenticated with **Employee ID + Password** (role = 1).

| Feature | Description |
|---|---|
| Add New Customer | Collect details, auto-assign account number, set opening balance |
| Modify Customer Details | Update first/last name of a customer |
| Approve / Reject Loans | Resolve an assigned loan; credits balance if approved |
| View Assigned Loans | List all pending loans assigned to this employee |
| View Customer Transactions | Look up any customer's transaction history |
| Change Password | Secure password update with file lock |
| Logout / Exit | Session cleanup with semaphore release |

### 🗂️ Manager
Authenticated with **Manager ID + Password** (role = 0).

| Feature | Description |
|---|---|
| Activate / Deactivate Account | Toggle customer account active status |
| Assign Loan to Employee | Route unassigned loan applications to employees |
| Review Customer Feedback | Read all submitted feedback |
| Change Password | Secure update with file locking |
| Logout / Exit | Clean session termination |

### 🔐 Admin
Authenticated with a **hardcoded master password** (`PASS@123`).

| Feature | Description |
|---|---|
| Add New Employee | Create employee record with auto-assigned ID |
| Modify Customer / Employee | Update name fields for any record |
| Manage User Roles | Promote employee to Manager or demote Manager to Employee |
| Logout | Return to main menu |

---

## Key Technical Concepts

### 1. TCP Client-Server with `fork()`
The server binds to port `8081` and uses `fork()` to spawn a child process for each accepted client. This gives every session its own process and memory space.

```c
connectionFileDescriptor = accept(...);
if (fork() == 0) {
    connectionHandler(connectionFileDescriptor);
}
```

### 2. `fcntl()` Record-Level File Locking
All write operations on shared binary files use `F_WRLCK` to lock only the specific record being modified — avoiding blocking unrelated operations in the same file.

```c
struct flock fl = { F_WRLCK, SEEK_SET, offset, sizeof(struct Customer), getpid() };
fcntl(file, F_SETLKW, &fl);   // Block until lock acquired
// ... modify record ...
fl.l_type = F_UNLCK;
fcntl(file, F_SETLK, &fl);    // Release lock
```

### 3. POSIX Named Semaphores (Duplicate Login Prevention)
Each user (Customer, Employee, Manager) gets a named semaphore `/sem_<ID>` initialized to `1`. A `sem_trywait()` "claims" the semaphore on login; if it returns `EAGAIN`, the user is already logged in. The semaphore is released on logout or process cleanup.

```c
sem_t *sema = sem_open("/sem_1001", O_CREAT, 0644, 1);
if (sem_trywait(sema) == -1 && errno == EAGAIN) {
    // Already logged in — reject
}
// On logout:
sem_post(sema);
sem_unlink("/sem_1001");
```

### 4. Signal Handling for Clean Semaphore Release
Signal handlers for `SIGINT`, `SIGTERM`, `SIGSEGV`, `SIGHUP`, and `SIGQUIT` are registered to ensure semaphores are always released even on abnormal termination.

### 5. Auto-Increment ID Generator (`FetchNextID.h`)
A file-locked, persistent ID counter in `Counter.txt` ensures unique IDs for Employees (starting 1), Customers (starting 1000), and Loans (starting 100) across restarts.

### 6. Password Masking on Client
The client disables terminal echo using `tcsetattr()` when reading passwords, so entered characters are not displayed.

### 7. Protocol: `'^'` as End-of-Message Signal
The server appends `'^'` to messages that require no further input from the client (info-only screens). The client detects this and sends an empty ACK, keeping the request-response loop synchronised.

---

## Workflow

### Customer Transaction Flow

```
Client                           Server
  │                                │
  │──── Login (AccNo + Password) ──►│  sem_trywait() + file scan
  │◄─── Customer Menu ─────────────│
  │──── Select: Deposit ───────────►│
  │◄─── "Enter amount:" ────────────│
  │──── Amount ─────────────────────►│  fcntl(F_WRLCK) on record
  │                                │  customer.balance += amount
  │                                │  write trans_hist entry
  │                                │  fcntl(F_UNLCK)
  │◄─── "Deposit successful!^" ─────│
  │──── Logout ─────────────────────►│  sem_post() + sem_unlink()
```

### Loan Lifecycle

```
Customer          Employee           Manager
    │                │                  │
    │ applyLoan()    │                  │
    │ status=0       │                  │
    │ (Requested)    │                  │
    │                │  assignLoan()    │
    │                │◄─────────────────│
    │                │  status=1        │
    │                │  (Pending)       │
    │  approveRejectLoan()              │
    │◄───────────────│                  │
    │  status=2 or 3                    │
    │  (Approved / Rejected)            │
```

---

## How to Build & Run

### Prerequisites
- Linux / Unix environment
- GCC compiler
- POSIX semaphore support (`-lpthread`)

### Compile

```bash
# Compile server
cd ClientAndServer
gcc -o server server.c -lpthread -lrt

# Compile client
gcc -o client client.c
```

### Run

```bash
# Terminal 1 — start the server
./server

# Terminal 2 (or more) — connect clients
./client
```

### Default Credentials

| Role | ID | Password |
|---|---|---|
| Admin | — | `PASS@123` |
| Employee / Manager | Set by Admin | Set on creation |
| Customer | Auto-assigned (≥1000) | Set by Employee |

---

## Use Cases

- Academic project for learning **System Programming in C** (sockets, IPC, semaphores, file I/O)
- Demonstrates **concurrent access control** in a multi-client server scenario
- Template for understanding **role-based access control (RBAC)** in file-backed systems
- Study material for OS-level concepts: `fork()`, `fcntl()`, `sem_open()`, `signal()`, `lseek()`

---

## Limitations & Future Scope

| Current Limitation | Possible Improvement |
|---|---|
| Passwords stored as plain text | Add `crypt()` / bcrypt hashing |
| Flat binary files as storage | Migrate to SQLite or PostgreSQL |
| No network encryption | Add TLS via OpenSSL |
| Single machine (localhost only) | Support remote IPs with proper `inet_addr()` binding |
| No web/GUI interface | Add a REST API layer or ncurses TUI |
| No audit logging | Log all admin/employee actions with timestamps |

---

## Author

Mini-project submitted for the **Software Systems** course.  
Built with C, Unix sockets, POSIX semaphores, and `fcntl()` file locking on Linux.
