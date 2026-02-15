#include "ServerEnum.h"
#include "MatchingServer.h"

int main() {
    MatchingServer matchingServer;
    matchingServer.Init();
    matchingServer.StartWork();

    std::cout << "========== ¸ÅÄª ¼­¹ö ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "matching") break;
    }

    matchingServer.ServerEnd();

    return 0;
}