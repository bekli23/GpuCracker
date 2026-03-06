#include "web_dashboard.h"

std::unique_ptr<WebDashboard> g_dashboard;

void initDashboard(int port) {
    g_dashboard = std::make_unique<WebDashboard>(port);
    g_dashboard->start();
}

void stopDashboard() {
    g_dashboard.reset();
}
