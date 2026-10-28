#pragma once

#include <optional>
#include <string>

namespace phoenix::frontend
{
    enum class FrontendState
    {
        Startup,
        Title,
        Login,
        ServerSelect,
        CharacterSelect,
        CharacterCreate,
        Options,
        Connecting,
        GameConnecting,
        GameLoading,
        Game,
        ErrorModal,
        ExitRequested
    };

    enum class FrontendEvent
    {
        BootComplete,
        OpenLogin,
        SubmitLogin,
        LoginSucceeded,
        LoginFailed,
        ServerListReady,
        OpenCharacterSelect,
        ReturnToCharacterSelect,
        StartGameConnecting,
        GameUpdateReceived,
        EnterGame,
        OpenCharacterCreate,
        CharacterCreateCompleted,
        OpenOptions,
        CloseOptions,
        Back,
        RequestExit
    };

    struct FrontendTransition
    {
        FrontendState from{ FrontendState::Startup };
        FrontendState to{ FrontendState::Startup };
        FrontendEvent event{ FrontendEvent::BootComplete };
    };

    class FrontendStateMachine final
    {
    public:
        [[nodiscard]] FrontendState State() const noexcept;
        [[nodiscard]] const std::optional<std::wstring>& ModalError() const noexcept;

        FrontendTransition Dispatch(FrontendEvent event);
        void SetModalError(std::wstring message);
        void ClearModalError() noexcept;

    private:
        FrontendState state_{ FrontendState::Startup };
        FrontendState returnState_{ FrontendState::Title };
        std::optional<std::wstring> modalError_;
    };
}
