#include "NetworkManager.h"
#include <iostream>

NetworkManager::NetworkManager(const std::string& host, int port, const std::string& facilityId)
    : defaultHost(host), defaultPort(port), currentFacilityId(facilityId) {
    server = std::make_unique<TcpServer>(defaultHost, defaultPort);
}

NetworkManager::~NetworkManager() {
    stopServer();
}

bool NetworkManager::startServer() {
    if (!server) {
        server = std::make_unique<TcpServer>(defaultHost, defaultPort);
    }
    return server->start();
}

void NetworkManager::stopServer() {
    if (server && server->isRunning()) {
        server->stop();
    }
}

bool NetworkManager::isServerRunning() const {
    return server ? server->isRunning() : false;
}

bool NetworkManager::sendFacilityUpdate(const std::string& facility, const std::string& medicine,
                                        const std::string& batch, int quantity, const std::string& type,
                                        std::string& ackResponse, std::string& errorMsg) {
    FacilityMessage msg;
    msg.facility = facility.empty() ? currentFacilityId : facility;
    msg.medicine = medicine;
    msg.batch = batch;
    msg.quantity = quantity;
    msg.type = type;

    TcpClient client(defaultHost, defaultPort);
    if (!client.sendFacilityUpdate(msg, ackResponse)) {
        errorMsg = client.getLastError();
        return false;
    }

    errorMsg = "";
    return true;
}

std::vector<FacilityMessage> NetworkManager::getStoredFacilityUpdates() const {
    if (server) {
        return server->getReceivedMessages();
    }
    return {};
}

size_t NetworkManager::getStoredUpdatesCount() const {
    if (server) {
        return server->getMessageCount();
    }
    return 0;
}

std::string NetworkManager::getHost() const {
    return defaultHost;
}

int NetworkManager::getPort() const {
    return defaultPort;
}

std::string NetworkManager::getFacilityId() const {
    return currentFacilityId;
}

void NetworkManager::setFacilityId(const std::string& id) {
    currentFacilityId = id;
}
