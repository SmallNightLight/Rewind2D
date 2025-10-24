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
        m_KeyActionCodes.fill(s_InvalidActionKey);
        m_MouseActionCodes.fill(s_InvalidActionKey);

        // Register by default mouse buttons
        for (int i = 1; i < 6; ++i)
        {
            RegisterMouseAction(i);
        }
    }

    inline ActionKey RegisterKeyAction(SDL_Scancode scanCode)
    {
        m_KeyActionCodes[scanCode] = m_NextAction;
        return ++m_NextAction;
    }

    inline ActionKey RegisterMouseAction(u_int8_t mouseButton)
    {
        m_MouseActionCodes[mouseButton] = m_NextAction;
        return ++m_NextAction;
    }

    inline bool HasKeyAction(SDL_Scancode scanCode) const
    {
        return m_KeyActionCodes[scanCode] != s_InvalidActionKey;
    }

    inline bool TryGetKeyAction(SDL_Scancode scanCode, ActionKey& outActionKey) const
    {
        if (!HasKeyAction(scanCode)) return false;

        outActionKey = m_KeyActionCodes[scanCode];
        return true; //Todo: add more complex input logic like multiple keys for 1 action
    }

    inline bool HasMouseAction(uint mouseButton) const
    {
        return m_MouseActionCodes[mouseButton] != s_InvalidActionKey;
    }

    inline bool TryGetMouseAction(uint mouseButton, ActionKey& outActionKey) const
    {
        if (!HasMouseAction(mouseButton)) return false;

        outActionKey = m_MouseActionCodes[mouseButton];
        return true;
    }

    inline ActionKey GetActionCount() const
    {
        return m_NextAction;
    }

private:
    ActionKey m_NextAction;
    std::array<ActionKey, SDL_Scancode::SDL_SCANCODE_COUNT> m_KeyActionCodes { };
    std::array<ActionKey, 256> m_MouseActionCodes { };
};