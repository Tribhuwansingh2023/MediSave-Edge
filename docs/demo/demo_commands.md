# MediSave Edge — Live Demo Command Sheet

This command sheet contains the exact commands required to execute the complete end-to-end trainer demonstration.

> **FULL RUN & VIVA GUIDE:**  
> For the complete demonstration walkthrough and evaluator Q&A defense, see [**`../../GUIDE.md`**](../../GUIDE.md).

---

## 1. Environment & Build Setup

```bash
# Navigate to project root
cd ~/MediSave-Edge

# Clean previous build artifacts
make clean

# Compile all applications and test runners
make all

# Verify generated binaries
ls -la bin/
```

---

## 2. Linux Kernel Character Driver Setup

```bash
# Compile the Linux loadable kernel module
make driver

# Insert kernel module into running kernel
sudo insmod driver/medisave_driver.ko

# Verify device registration
ls -l /dev/medisave

# Set temporary prototype testing permission:
sudo chmod 666 /dev/medisave
# Production alternative (udev rule):
# echo 'KERNEL=="medisave", MODE="0660", GROUP="dialout"' | sudo tee /etc/udev/rules.d/99-medisave.rules

# Check kernel ring buffer log
dmesg | tail -n 15
```

---

## 3. Run Automated Test Verification

```bash
# Run all 8 automated unit & integration test suites
make test

# Optional: Run specific test suites individually
make driver-test
make redistribution-test
make system-monitor-test
```

---

## 4. Standalone Multi-Facility TCP Server & Client Demo (Optional)

```bash
# Terminal 1: Launch Standalone TCP Server
./bin/medisave_server

# Terminal 2: Dispatch Surplus from Facility-A
./bin/medisave_client Facility-A Paracetamol P2026A 150 SURPLUS

# Terminal 3: Dispatch Shortage from Facility-B
./bin/medisave_client Facility-B Paracetamol P2026A 20 SHORTAGE
```

---

## 5. Main MediSave Edge Application Execution

```bash
# Launch interactive CLI
./bin/medisave
```

### Recommended In-App Menu Sequence:
1. **Option 5** — Display Inventory Table
2. **Option 7** — Check Expiry Report
3. **Option 8** — View Prioritized Alert Queue (Max-Heap)
4. **Option 9** — Read Live Storage Temperature (Default: 6.50 °C - NORMAL)
5. **Option 10** — Set Simulated Temperature to `11.50` °C
6. **Option 11** — Show Storage Condition (Evaluates CRITICAL, alerts breached drugs)
7. **Option 12** — Start Background Monitoring (Threads + TCP server)
8. **Option 14** — Send Facility Update:
   * Facility: `Facility-A`, Medicine: `Paracetamol`, Batch: `P2026A`, Qty: `150`, Type: `1 (SURPLUS)`
9. **Option 14** — Send Facility Update:
   * Facility: `Facility-B`, Medicine: `Paracetamol`, Batch: `P2026A`, Qty: `20`, Type: `2 (SHORTAGE)`
10. **Option 15** — Analyze Redistribution (Matches Facility-A -> Facility-B for 30 units)
11. **Option 16** — Show System Health (Inspects live `/proc` CPU, RAM, Uptime)
12. **Option 17** — Show System Dashboard (Consolidated single-screen overview)
13. **Option 13** — Stop Background Monitoring
14. **Option 18** — Exit Application (Gracefully saves persistence file and shuts down)

---

## 6. Teardown & Clean Up

```bash
# Unload Linux kernel module
sudo rmmod medisave_driver

# Verify device node removal
ls -l /dev/medisave 2>/dev/null || echo "Driver unloaded successfully."

# Verify kernel teardown log
dmesg | tail -n 5
```
