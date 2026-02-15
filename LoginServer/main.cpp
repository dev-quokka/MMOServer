#include "LoginServer.h"

int main() {
    LoginServer loginServer;

    loginServer.init();
    loginServer.StartWork();

    std::cout << "========== 로그인 서버 ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "login") break;
    }

    loginServer.ServerEnd();

    return 0;
}