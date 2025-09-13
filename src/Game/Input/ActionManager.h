#pragma once

#include <array>
#include <cstdint>
#include <SDL3/SDL_scancode.h>

using ActionKey = uint8_t;
static constexpr ActionKey s_InvalidActionKey = 255;

class ActionManager
{
public:
    ActionManager() : NextAction(0)
    {
        ActionCodes.fill(s_InvalidActionKey);
    }

    inline ActionKey RegisterAction(SDL_Scancode scanCode)
    {
        ActionCodes[scanCode] = NextAction;
        return ++NextAction;
    }

    inline bool HasActionKey(SDL_Scancode scanCode) const
    {
        return ActionCodes[scanCode] != s_InvalidActionKey;
    }

    inline bool TryGetActionKey(SDL_Scancode scanCode, ActionKey& outActionKey) const
    {
        if (!HasActionKey(scanCode)) return false;

        outActionKey = ActionCodes[scanCode];
        return true; //Todo: add more complex input logic like multiple keys for 1 action
    }

    ActionKey NextAction;
    std::array<ActionKey, SDL_Scancode::SDL_SCANCODE_COUNT> ActionCodes { };
};