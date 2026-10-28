#include "Frontend/FrontendStateMachine.h"

#include <utility>

namespace phoenix::frontend
{
    FrontendState FrontendStateMachine::State() const noexcept
    {
        return state_;
    }

    const std::optional<std::wstring>& FrontendStateMachine::ModalError() const noexcept
    {
        return modalError_;
    }

    FrontendTransition FrontendStateMachine::Dispatch(const FrontendEvent event)
    {
        const auto from = state_;

        switch (event)
        {
        case FrontendEvent::BootComplete:
            state_ = FrontendState::Title;
            break;
        case FrontendEvent::OpenLogin:
            state_ = FrontendState::Login;
            break;
        case FrontendEvent::SubmitLogin:
            if (state_ == FrontendState::Login)
            {
                state_ = FrontendState::Connecting;
            }
            break;
        case FrontendEvent::LoginSucceeded:
            if (state_ == FrontendState::Connecting)
            {
                state_ = FrontendState::ServerSelect;
            }
            break;
        case FrontendEvent::ServerListReady:
            if (state_ == FrontendState::Connecting)
            {
                state_ = FrontendState::ServerSelect;
            }
            break;
        case FrontendEvent::LoginFailed:
            returnState_ = FrontendState::Login;
            state_ = FrontendState::ErrorModal;
            break;
        case FrontendEvent::OpenCharacterSelect:
            if (state_ == FrontendState::ServerSelect)
            {
                state_ = FrontendState::CharacterSelect;
            }
            break;
        case FrontendEvent::ReturnToCharacterSelect:
            if (state_ == FrontendState::Game ||
                state_ == FrontendState::GameConnecting ||
                state_ == FrontendState::GameLoading)
            {
                state_ = FrontendState::CharacterSelect;
            }
            break;
        case FrontendEvent::StartGameConnecting:
            if (state_ == FrontendState::CharacterSelect)
            {
                state_ = FrontendState::GameConnecting;
            }
            break;
        case FrontendEvent::GameUpdateReceived:
            if (state_ == FrontendState::GameConnecting)
            {
                state_ = FrontendState::GameLoading;
            }
            break;
        case FrontendEvent::EnterGame:
            if (state_ == FrontendState::GameConnecting || state_ == FrontendState::GameLoading)
            {
                state_ = FrontendState::Game;
            }
            break;
        case FrontendEvent::OpenCharacterCreate:
            if (state_ == FrontendState::CharacterSelect)
            {
                state_ = FrontendState::CharacterCreate;
            }
            break;
        case FrontendEvent::CharacterCreateCompleted:
            if (state_ == FrontendState::CharacterCreate)
            {
                state_ = FrontendState::CharacterSelect;
            }
            break;
        case FrontendEvent::OpenOptions:
            returnState_ = state_;
            state_ = FrontendState::Options;
            break;
        case FrontendEvent::CloseOptions:
            state_ = returnState_;
            break;
        case FrontendEvent::Back:
            if (state_ == FrontendState::ErrorModal)
            {
                ClearModalError();
                state_ = returnState_;
            }
            else if (state_ == FrontendState::Login)
            {
                state_ = FrontendState::Title;
            }
            else if (state_ == FrontendState::ServerSelect)
            {
                state_ = FrontendState::Login;
            }
            else if (state_ == FrontendState::CharacterSelect)
            {
                state_ = FrontendState::ServerSelect;
            }
            else if (state_ == FrontendState::CharacterCreate)
            {
                state_ = FrontendState::CharacterSelect;
            }
            else if (state_ == FrontendState::GameConnecting || state_ == FrontendState::GameLoading)
            {
                state_ = FrontendState::CharacterSelect;
            }
            break;
        case FrontendEvent::RequestExit:
            state_ = FrontendState::ExitRequested;
            break;
        }

        return FrontendTransition{ from, state_, event };
    }

    void FrontendStateMachine::SetModalError(std::wstring message)
    {
        modalError_ = std::move(message);
    }

    void FrontendStateMachine::ClearModalError() noexcept
    {
        modalError_.reset();
    }
}
