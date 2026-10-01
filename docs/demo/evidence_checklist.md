# MediSave Edge — Demonstration & Evaluation Evidence Checklist

This document outlines the 14 verified evidence points recommended for terminal capture, presentation slides, or evaluation dossiers.

---

## Evidence Checkpoints

| # | Evidence Point | Command / Action | Expected Terminal Output | Status |
| :---: | :--- | :--- | :--- | :---: |
| **01** | **GitHub Repository** | `git status && git log -n 5` | Clean working tree on `main` branch with atomic commit history. | **READY** |
| **02** | **Project Structure** | `tree -L 3` or `cat docs/project_structure.txt` | Complete modular directory layout matching documented architecture. | **READY** |
| **03** | **Clean Build** | `make clean && make all` | Zero warnings, zero errors; all executables and test runners built. | **READY** |
| **04** | **Kernel Driver Build** | `cd driver && make && ls -l medisave_driver.ko` | Clean compilation of `medisave_driver.ko` against kernel build tree. | **READY** |
| **05** | **Device Node Creation** | `sudo insmod driver/medisave_driver.ko && ls -l /dev/medisave` | Character device node created (mode 0666 temporary / mode 0660 udev). | **READY** |
| **06** | **Normal Temperature** | `./bin/medisave` -> Option 9 | `Temperature: 6.50 C`, `Status: NORMAL`. | **READY** |
| **07** | **Critical Excursion** | `./bin/medisave` -> Option 10 (11.50 °C) -> Option 11 | `Temperature: 11.50 C`, `Status: CRITICAL`, storage excursion alert. | **READY** |
| **08** | **Inventory Table** | `./bin/medisave` -> Option 5 | Formatted ASCII table displaying 25 medicine catalog records with stock and limits. | **READY** |
| **09** | **Expiry Alert Queue** | `./bin/medisave` -> Option 8 | Max-Heap prioritization of expired batches and $\le 7$ day expiries. | **READY** |
| **10** | **TCP Facility Server** | `./bin/medisave_server` | Server listening on `127.0.0.1:5000` with concurrent client threads. | **READY** |
| **11** | **TCP Facility Client** | `./bin/medisave_client Facility-A Paracetamol P2026A 150 SURPLUS` | Client payload transmission and reception of `ACK|Facility-A`. | **READY** |
| **12** | **Redistribution Engine** | `./bin/medisave` -> Option 15 | Advisory proposal: `Facility-A -> Facility-B (30 units)` with disclaimer. | **READY** |
| **13** | **Executive Dashboard** | `./bin/medisave` -> Option 17 | Unified single-screen overview of Storage, Inventory, Redist, and Host. | **READY** |
| **14** | **Automated Test Results** | `make test` | All 8 automated test suites passing with 100% success rate. | **READY** |
