#include "UI/Text/LoginTextLayout.h"

namespace phoenix::ui::text
{
    namespace
    {
        std::wstring WidenAscii(const std::string& text)
        {
            std::wstring widened;
            widened.reserve(text.size());
            for (const auto ch : text)
            {
                widened.push_back(static_cast<wchar_t>(static_cast<unsigned char>(ch)));
            }
            return widened;
        }
    }

    std::vector<LoginTextRun> LoginTextLayout::Build(const ui::LoginInputModel& input)
    {
        const auto& layout = input.Layout();
        std::vector<LoginTextRun> runs;
        runs.push_back(LoginTextRun{ layout.idEdit, WidenAscii(input.Username()), false });
        runs.push_back(LoginTextRun{ layout.passwordEdit, std::wstring(input.Password().size(), L'*'), true });
        return runs;
    }
}
