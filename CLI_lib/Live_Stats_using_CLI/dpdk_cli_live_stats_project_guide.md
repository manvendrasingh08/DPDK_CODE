# DPDK CLI Live Stats Project

## 1. Overview

This project demonstrates a basic multi-core DPDK application with a command-line interface (CLI). It separates:

- **Control Plane** → CLI (user interaction)
- **Data Plane** → RX/TX loops (packet processing simulation)

The application shows how to:
- Build a modular CLI using DPDK cmdline library
- Run tasks on multiple lcores (CPU cores)
- Maintain and display live statistics

---

## 2. Folder Structure

```
Live_Stats_using_CLI/
├── main.c
├── stats.h
├── stats.c
├── rx.c
├── tx.c
├── cli/
│   ├── cli.h
│   ├── cli.c
│   ├── cmd_show.c
│   ├── cmd_set.c
│   ├── cmd_exit.c
├── Makefile
```

---

## 3. File Responsibilities

### main.c
- Initializes DPDK EAL
- Launches RX and TX loops on separate lcores
- Starts CLI on main core

### stats.h / stats.c
- Defines global statistics structure
- Stores RX/TX packet counters

### rx.c
- Simulates packet reception
- Continuously increments RX counters

### tx.c
- Simulates packet transmission
- Continuously increments TX counters

### cli/cli.h
- Declares all CLI commands

### cli/cli.c
- Registers all CLI commands into a single context

### cli/cmd_show.c
- Implements: `show port stats`
- Displays live statistics

### cli/cmd_set.c
- Implements: `set port <id> up/down`
- Currently prints action (can be extended to control dataplane)

### cli/cmd_exit.c
- Implements: `exit`
- Gracefully exits CLI

### Makefile
- Compiles all source files
- Links against DPDK libraries

---

## 4. Build Instructions

Run the following commands inside the project directory:

```
make clean
make
```

This generates the executable:

```
./live_stats
```

---

## 5. Run Instructions

Run with multiple cores:

```
sudo ./live_stats -l 0-2 -n 1
```

### Meaning:
- `-l 0-2` → use cores 0, 1, 2
- `-n 1` → memory channels

---

## 6. Startup Output

```
Starting DPDK multi-core app...
Launching RX on lcore 1
Launching TX on lcore 2
dpdk>
```

---

## 7. CLI Commands

### 1. Show Stats

```
dpdk> show port stats
```

**Output:**
```
==== PORT STATS ====
Port 0: RX=1000 TX=800 DROP=0
Port 1: RX=0 TX=0 DROP=0
```

---

### 2. Set Port State

```
dpdk> set port 1 up
```

```
dpdk> set port 1 down
```

**Output:**
```
Setting port 1 to up
```

---

### 3. Exit CLI

```
dpdk> exit
```

**Output:**
```
Exiting CLI...
```

---

## 8. Execution Flow

```
Main Core (0)
   → Runs CLI

Lcore 1
   → RX loop updates rx_pkts

Lcore 2
   → TX loop updates tx_pkts

Shared Memory
   → stats structure

CLI
   → reads stats and prints
```

---

## 9. Key Concepts Demonstrated

- Multi-core execution using `rte_eal_remote_launch`
- Separation of control plane and data plane
- Token-based CLI parsing
- Shared memory for stats

---

## 10. Current Limitations

- No real NIC interaction (simulated RX/TX)
- Port up/down does not control dataplane yet
- No synchronization for stats (acceptable for demo)

---

## 11. Possible Extensions

- Add real NIC support (`rte_eth_dev_start`)
- Make `set port up/down` control RX/TX loops
- Add more CLI commands
- Add per-queue statistics

---

## 12. Summary

This project is a foundational example of building a scalable DPDK application with:
- Modular CLI
- Multi-core processing
- Real-time statistics

It closely follows patterns used in production-grade DPDK applications.

