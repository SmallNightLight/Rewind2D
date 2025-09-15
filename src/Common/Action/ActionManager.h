#pragma once

#include <array>
#include <cstdint>
#include <SDL3/SDL_scancode.h>

using ActionKey = uint8_t;
static constexpr ActionKey s_InvalidActionKey = 255;

class ActionManager
{
public:
    ActionManager() : m_NextAction(0)
    {
        m_ActionCodes.fill(s_InvalidActionKey);
    }

    inline ActionKey RegisterAction(SDL_Scancode scanCode)
    {
        m_ActionCodes[scanCode] = m_NextAction;
        return ++m_NextAction;
    }

    inline bool HasActionKey(SDL_Scancode scanCode) const
    {
        return m_ActionCodes[scanCode] != s_InvalidActionKey;
    }

    inline bool TryGetActionKey(SDL_Scancode scanCode, ActionKey& outActionKey) const
    {
        if (!HasActionKey(scanCode)) return false;

        outActionKey = m_ActionCodes[scanCode];
        return true; //Todo: add more complex input logic like multiple keys for 1 action
    }

    inline ActionKey GetActionCount() const
    {
        return m_NextAction;
    }

private:
    ActionKey m_NextAction;
    std::array<ActionKey, SDL_Scancode::SDL_SCANCODE_COUNT> m_ActionCodes { };
};