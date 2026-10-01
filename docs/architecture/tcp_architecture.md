# TCP Client/Server Architecture & Inter-Facility Protocol

## MediSave Edge Network Subsystem

This document specifies the TCP networking model, socket lifecycle, text wire protocol, multi-client concurrency, and error recovery implemented in **Task 6** of **MediSave Edge**.

---

## 1. Architectural Topology

MediSave Edge simulates a distributed health network where multiple regional storage facilities (or nodes) communicate inventory availability with a central coordination server.

```
       FACILITY A CLIENT                         FACILITY B CLIENT
      (bin/medisave_client)                    (bin/medisave_client)
               |                                        |
               | TCP Socket                             | TCP Socket
               | (127.0.0.1:5000)                       | (127.0.0.1:5000)
               v                                        v
       +--------------------------------------------------------+
       |               MEDISAVE CENTRAL TCP SERVER              |
       |                  (bin/medisave_server)                 |
       |                                                        |
       |  Accept Loop: socket() -> bind() -> listen() -> accept |
       |  Worker Pool: std::thread per accepted client socket   |
       |  Storage: Thread-Safe In-Memory FacilityMessage Store   |
       +--------------------------------------------------------+
                                   |
                                   v
             [Prepared for Task 7 Redistribution Engine]
```

---

## 2. Socket Lifecycle

### Server Lifecycle (`TcpServer`)
1. **Creation:** `socket(AF_INET, SOCK_STREAM, 0)` generates an IPv4 TCP stream socket.
2. **Reuse Configuration:** `setsockopt(SO_REUSEADDR)` permits immediate port re-binding without waiting for `TIME_WAIT` expiration.
3. **Binding:** `bind()` associates the socket with the configured interface (default `127.0.0.1`) and port (`5000`).
4. **Passive Listen:** `listen(serverSocket, 16)` marks the socket as receptive to incoming connection requests.
5. **Accept Loop (`acceptThread`):** Blocks on `accept()`. When a connection arrives:
   - Extracts client IP address.
   - Spawns a dedicated worker thread (`std::thread(&TcpServer::handleClient, ...)`) to process the transaction concurrently.
6. **Clean Shutdown:** Closing `serverSocket` unblocks `accept()`. The server then waits for all client worker threads to finish (`th.join()`).

### Client Lifecycle (`TcpClient`)
1. **Connection:** Creates socket and executes blocking `connect()` against server endpoint.
2. **Transmission:** Formats wire protocol payload and issues `send()`.
3. **Acknowledgement Receipt:** Reads server response using `recv()`.
4. **Orderly Disconnect:** Explicitly invokes `close()` (`closesocket()` on Windows) to terminate the transaction.

---

## 3. Wire Message Protocol

MediSave Edge uses a lightweight, human-readable, pipe-delimited text protocol:

### Facility Update Format
```text
FACILITY|MEDICINE|BATCH|QUANTITY|TYPE\n
```

| Field | Type | Description | Example |
| :--- | :--- | :--- | :--- |
| `FACILITY` | String | Unique identifier of originating hospital/storage unit | `Facility-A` |
| `MEDICINE` | String | Name of medicine | `Paracetamol` |
| `BATCH` | String | Manufacturer batch code | `P2026A` |
| `QUANTITY` | Integer | Quantity in units | `150` |
| `TYPE` | String | Transaction category (`SURPLUS` or `SHORTAGE`) | `SURPLUS` |

### Server Response
```text
ACK|Facility-A\n
```
Or upon parsing errors:
```text
ERR|INVALID_FORMAT\n
```

---

## 4. Multi-Client Concurrency Model

- **Thread-per-Client:** Each accepted TCP connection is processed on an independent `std::thread`, preventing a slow or stalled client from blocking other facilities.
- **Mutex Synchronization:** The server records incoming facility reports into `std::vector<FacilityMessage> receivedMessages` protected by `mutable std::mutex messagesMutex`.
- **Zero Heap Leakage:** All worker threads are registered in `std::vector<std::thread> clientThreads` and joined cleanly during server termination.

---

## 5. Socket Error Handling

| Failure Scenario | Mitigation / Application Behavior |
| :--- | :--- |
| Server unavailable | `TcpClient` catches `connect()` failure, reports `Unable to connect to MediSave server on 127.0.0.1:5000. Please start the TCP server first.`, and returns `false` without crashing. |
| Port collision / already in use | `bind()` fails with `EADDRINUSE` / `WSAEADDRINUSE`. `TcpServer::start()` returns `false` and reports the error cleanly. |
| Premature client disconnect | `recv()` returns $\le 0$; server logs disconnect and frees client socket descriptor. |
| Malformed payload | Parser detects missing delimiter tokens and responds with `ERR|INVALID_FORMAT`. |
