#pragma once

struct UserInformation
{
    bool IsNetmarble{ false };
    bool IsNetmarbleLogin{ false };
    bool IsNetmarbleJapan{ false };
};

extern UserInformation* g_pUserInformation;
