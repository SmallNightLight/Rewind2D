#pragma once

#include "Action.h"

#include <cstdint>
#include <vector>
#include <algorithm>
#include <cassert>
#include <stdexcept>

class ActionCollection
{
public:
    explicit ActionCollection(uint32_t frame = 0, uint32_t size = 15) : m_SaveCount(size), m_Actions(size), m_OldestFrame(frame), m_StartIndex(0), m_FrameCount(0), m_LastCompletedFrame(frame)
    {
        if (size == 0)
            throw std::invalid_argument("SaveCount must be greater than 0");
    }

    void AddAction(const Action& action)
    {
        if (action.Frame < m_OldestFrame)
        {
            //Most likely received an old packet from another client that just started
            return;
        }

        assert(action.Frame >= m_OldestFrame && action.Frame < m_OldestFrame + m_SaveCount * 2 && "Frame is outside the valid range of the buffer");

        if (HasAction(action.Frame))
        {
            if (action.Frame == m_LastCompletedFrame + 1)
                ++m_LastCompletedFrame;

            return;
        }

        m_Actions[GetIndex(action.Frame)] = action;

        if (m_FrameCount < m_SaveCount)
        {
            m_FrameCount = std::min(action.Frame, m_SaveCount);
        }
        else if (action.Frame >= m_OldestFrame + m_SaveCount)
        {
            //Calculate how many frames the buffer is advancing
            uint32_t framesAdvanced = action.Frame - (m_OldestFrame + m_SaveCount - 1);

            //Update the oldestFrame and adjust startIndex accordingly
            m_OldestFrame = action.Frame - m_SaveCount + 1;
            m_StartIndex = (m_StartIndex + framesAdvanced) % m_SaveCount;
        }

        //Update the last complete frame if the new action fills a gap
        if (action.Frame == m_LastCompletedFrame + 1)
            ++m_LastCompletedFrame;
    }

    void JumpToFrame(uint32_t frame)
    {
        m_OldestFrame = frame;
        m_StartIndex = 0;
        m_FrameCount = std::min(frame, m_SaveCount);
        m_LastCompletedFrame = frame;
    }

    //Retrieves the action for a specific frame
    [[nodiscard]] Action GetAction(uint32_t frame) const
    {
        if (!HasAction(frame))
        {
            return GetPredictedAction(frame);
        }

        return m_Actions[GetIndex(frame)];
    }

    [[nodiscard]] Action GetPredictedAction(uint32_t frame) const
    {
        //Get last registered action
        uint32_t index = GetIndex(m_LastCompletedFrame);
        Action action = m_Actions[index];
        action.Frame = frame;
        return action;
    }

    [[nodiscard]] bool NeedsPrediction(uint32_t frame) const
    {
        return !HasAction(frame);
    }

    [[nodiscard]] bool HasAction(uint32_t frame) const
    {
        //Check if the frame is within the range of the buffer
        return frame >= m_OldestFrame && frame < m_OldestFrame + m_FrameCount && m_FrameCount > 0;
    }

    [[nodiscard]] uint32_t GetRollbackFrame(uint32_t currentFrame) const
    {
        assert(currentFrame >= m_LastCompletedFrame && "Current frame is in the past?");

        return currentFrame - m_LastCompletedFrame;
    }

    [[nodiscard]]uint32_t GetLastCompletedFrame() const
    {
        return m_LastCompletedFrame;
    }

private:
    [[nodiscard]] uint32_t GetIndex(uint32_t frame) const
    {
        return (m_StartIndex + (frame - m_OldestFrame)) % m_SaveCount;
    }

private:
    uint32_t m_SaveCount;                     //Maximum number of frames to save
    std::vector<Action> m_Actions;            //Stores the actions
    uint32_t m_OldestFrame;                   //Oldest frame where the action it still saved
    uint32_t m_StartIndex;                    //Index of the oldest frame in the actions, since it is circular
    uint32_t m_FrameCount;                    //Number of frames currently stored in the buffer
    uint32_t m_LastCompletedFrame;            //The highest frame where all previous actions are known
};