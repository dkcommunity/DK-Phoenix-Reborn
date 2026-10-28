#include "Platform/Application.h"

#include <Windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCommand)
{
    phoenix::platform::Application application(instance);
    return application.Run(showCommand);
}
