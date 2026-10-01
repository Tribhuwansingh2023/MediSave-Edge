# ==============================================================================
# MediSave Edge - Master Makefile
# Linux-Based Medicine Storage Monitoring, Inventory Alert and Redistribution System
# ==============================================================================

CXX      ?= g++
CXXFLAGS ?= -Wall -Wextra -O2 -g -std=c++17 -pthread
INCLUDES := -Iinclude

UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
LDFLAGS := -pthread
ifeq ($(UNAME_S),Linux)
    LDFLAGS += -lrt
endif

SRC_DIR    := src
BUILD_DIR  := build
BIN_DIR    := bin
TESTS_DIR  := tests
DRIVER_DIR := driver

# Core source files
CORE_SRCS := $(SRC_DIR)/medicine.cpp \
             $(SRC_DIR)/inventory_manager.cpp \
             $(SRC_DIR)/expiry_utils.cpp \
             $(SRC_DIR)/alert_system.cpp \
             $(SRC_DIR)/DeviceSensor.cpp \
             $(SRC_DIR)/StorageMonitor.cpp \
             $(SRC_DIR)/IPCManager.cpp \
             $(SRC_DIR)/ProcessManager.cpp

CORE_OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CORE_SRCS))

# Binaries
APP_BIN          := $(BIN_DIR)/medisave
WORKER_BIN       := $(BIN_DIR)/monitor_worker
TEST_BIN         := $(BIN_DIR)/test_inventory
SENSOR_TEST_BIN  := $(BIN_DIR)/device_sensor_test
DRIVER_TEST_BIN  := $(BIN_DIR)/driver_test
IPC_TEST_BIN     := $(BIN_DIR)/ipc_test
PROCESS_TEST_BIN := $(BIN_DIR)/process_test

.PHONY: all run test clean distclean help dirs driver driver-clean ipc-test process-test driver-test

all: dirs $(APP_BIN) $(WORKER_BIN) $(IPC_TEST_BIN) $(PROCESS_TEST_BIN) $(TEST_BIN) $(SENSOR_TEST_BIN) $(DRIVER_TEST_BIN)

dirs:
	@mkdir -p $(BUILD_DIR) $(BIN_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Main MediSave Edge CLI Executable
$(APP_BIN): $(CORE_OBJS) $(SRC_DIR)/main.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Main application successfully compiled: $(APP_BIN)

# Monitor Worker Process Executable (fork/exec target)
$(WORKER_BIN): $(SRC_DIR)/monitor_worker.cpp $(BUILD_DIR)/DeviceSensor.o $(BUILD_DIR)/StorageMonitor.o $(BUILD_DIR)/IPCManager.o | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Monitor worker successfully compiled: $(WORKER_BIN)

# Task 2 Inventory Unit Tests
$(TEST_BIN): $(CORE_OBJS) $(TESTS_DIR)/test_inventory.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Inventory test runner successfully compiled: $(TEST_BIN)

# Task 4 Device Sensor Integration Tests
$(SENSOR_TEST_BIN): $(BUILD_DIR)/DeviceSensor.o $(BUILD_DIR)/StorageMonitor.o $(TESTS_DIR)/device_sensor_test.cpp | dirs
	$(CXX) -Wall -Wextra -O1 -g -std=c++17 -pthread $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Device sensor test successfully compiled: $(SENSOR_TEST_BIN)

# Task 5 IPC Test Program
$(IPC_TEST_BIN): $(TESTS_DIR)/ipc_test.cpp $(BUILD_DIR)/IPCManager.o | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] IPC test successfully compiled: $(IPC_TEST_BIN)

# Task 5 Process Lifecycle Test Program
$(PROCESS_TEST_BIN): $(TESTS_DIR)/process_test.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Process test successfully compiled: $(PROCESS_TEST_BIN)

# Task 3/4 Direct Driver Test Program
$(DRIVER_TEST_BIN): $(BUILD_DIR)/DeviceSensor.o $(TESTS_DIR)/driver_test.cpp | dirs
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@
	@echo [Build] Driver test successfully compiled: $(DRIVER_TEST_BIN)

ipc-test: $(IPC_TEST_BIN)
	@./$(IPC_TEST_BIN)

process-test: $(PROCESS_TEST_BIN) $(WORKER_BIN)
	@./$(PROCESS_TEST_BIN)

driver-test: $(DRIVER_TEST_BIN)
	@./$(DRIVER_TEST_BIN)

test: $(TEST_BIN) $(SENSOR_TEST_BIN) $(IPC_TEST_BIN) $(PROCESS_TEST_BIN) $(WORKER_BIN)
	@echo "=========================================="
	@echo " Running MediSave Edge Unit Tests..."
	@echo "=========================================="
	@./$(TEST_BIN)
	@echo "=========================================="
	@echo " Running Device Sensor Integration Tests..."
	@echo "=========================================="
	@./$(SENSOR_TEST_BIN)
	@echo "=========================================="
	@echo " Running IPC (Pipe, Shm, Sem) Tests..."
	@echo "=========================================="
	@./$(IPC_TEST_BIN)
	@echo "=========================================="
	@echo " Running Process (fork, exec, waitpid) Tests..."
	@echo "=========================================="
	@./$(PROCESS_TEST_BIN)

driver:
	@$(MAKE) -C $(DRIVER_DIR) all

driver-clean:
	@$(MAKE) -C $(DRIVER_DIR) clean

run: $(APP_BIN)
	@./$(APP_BIN)

clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@$(MAKE) -C $(DRIVER_DIR) clean > /dev/null 2>&1 || true
	@echo [Clean] Build and binary directories removed.

distclean: clean
	@rm -f *.log *.tmp
	@echo [DistClean] Complete cleanup done.

help:
	@echo MediSave Edge Build Commands:
	@echo   make              - Compile all binaries (app, worker, tests)
	@echo   make run          - Compile and launch bin/medisave
	@echo   make test         - Compile and run all 4 test suites
	@echo   make ipc-test     - Compile and run IPC test suite
	@echo   make process-test - Compile and run process lifecycle test
	@echo   make driver       - Build Linux kernel character driver module
	@echo   make driver-clean - Clean driver kernel module build artifacts
	@echo   make clean        - Remove build artifacts and binaries
