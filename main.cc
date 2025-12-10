#include <drogon/drogon.h>
#include <filesystem>
#include <cstdlib>

int main() {
    // Загружаем конфиг: приоритет у переменной окружения DROGON_CONFIG,
    // иначе дефолтный ../config.json (относительно build/).
    std::string configPath = "../config.json";
    if (const char* envCfg = std::getenv("DROGON_CONFIG")) {
        if (std::filesystem::exists(envCfg)) {
            configPath = envCfg;
        }
    }
    drogon::app().loadConfigFile(configPath);

    // Если в конфиге уже есть listeners, эту строку можно не вызывать,
    // но она не мешает и переопределяет адрес/порт при необходимости.
    drogon::app().addListener("0.0.0.0", 9000);

    drogon::app().run();
    return 0;
}
