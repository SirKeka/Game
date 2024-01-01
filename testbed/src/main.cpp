#include <core/application.hpp>

int main()
{
    ApplicationConfig Config;
    Config.name = "Moon Engine";
    Config.StartPosX = 100;
    Config.StartPosY = 100;
    Config.StartHeight = 1280;
    Config.StartWidth = 720;
    

    ApplicationCreate(&Config);
    ApplicationRun();

    return 0;
}