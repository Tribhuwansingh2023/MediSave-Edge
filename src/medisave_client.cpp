#include "TcpClient.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::string facility = "Facility-A";
    std::string medicine = "Paracetamol";
    std::string batch = "P2026A";
    int quantity = 150;
    std::string type = "SURPLUS";

    if (argc >= 2) {
        facility = argv[1];
        if (facility == "Facility-B") {
            quantity = 20;
            type = "SHORTAGE";
        }
    }
    if (argc >= 3) {
        medicine = argv[2];
    }
    if (argc >= 4) {
        batch = argv[3];
    }
    if (argc >= 5) {
        try {
            quantity = std::stoi(argv[4]);
        } catch (...) {
            quantity = 50;
        }
    }
    if (argc >= 6) {
        type = argv[5];
    }

    std::cout << "========================================\n";
    std::cout << "       MEDISAVE TCP CLIENT\n";
    std::cout << "========================================\n\n";

    std::cout << "Target Server : " << DEFAULT_TCP_HOST << ":" << DEFAULT_TCP_PORT << "\n";
    std::cout << "Facility ID   : " << facility << "\n";
    std::cout << "Medicine      : " << medicine << "\n";
    std::cout << "Batch Number  : " << batch << "\n";
    std::cout << "Quantity      : " << quantity << "\n";
    std::cout << "Update Type   : " << type << "\n\n";

    FacilityMessage msg{facility, medicine, batch, quantity, type};
    std::cout << "Sending: " << serializeFacilityMessage(msg);

    TcpClient client(DEFAULT_TCP_HOST, DEFAULT_TCP_PORT);
    std::string ack;

    if (client.sendFacilityUpdate(msg, ack)) {
        std::cout << "\nServer acknowledgement received: " << ack << "\n";
        std::cout << "Transaction completed successfully.\n";
        std::cout << "========================================\n";
        return 0;
    } else {
        std::cerr << "\n[ERROR] " << client.getLastError() << "\n";
        std::cout << "========================================\n";
        return 1;
    }
}
