#include "GameServer1.h"

int main() {
	GameServer1 gameServer1;

    if (!gameServer1.init()) {
        return 0;
    }

    gameServer1.StartWork();

    std::cout << "========== 게임 서버1 ==========" << std::endl;
    std::string k = "";

    while (1) {
        std::cin >> k;
        if (k == "game1") break;
    }

    gameServer1.ServerEnd();

	return 0;
}