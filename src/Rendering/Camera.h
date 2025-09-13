#pragma once

#include "../Math/FixedTypes.h"

#include <SDL3/SDL_rect.h>

struct Camera
{
    Fixed16_16 Width, Height;
    Fixed16_16 ZoomLevel;
    Vector2 Position;

    //Cache
    Fixed16_16 Left, Right, Bottom, Top;
    Fixed16_16 ScaleX, ScaleY;

    inline Camera() noexcept = default;

    constexpr inline explicit Camera(Fixed16_16 width, Fixed16_16 height, Fixed16_16 zoomLevel = Fixed16_16(1))
        : Width(width), Height(height), ZoomLevel(zoomLevel), Position(0, 0), Left(0), Right(0), Bottom(0), Top(0), ScaleX(0), ScaleY(0)
    {
        UpdateView();
    }

    Vector2 WorldToScreen(const Vector2& world) const
    {
        return Vector2((world.X - Left) * ScaleX, (world.Y - Top) * ScaleY);
    }

    SDL_FPoint WorldToScreen(const  SDL_FPoint world) const
    {
        return SDL_FPoint {(world.x - Left.ToFloating<float>()) * ScaleX.ToFloating<float>(), (world.y - Top.ToFloating<float>()) * ScaleY.ToFloating<float>() };
    }

    void SetPosition(Vector2 position)
    {
        Position = position;
        UpdateView();
    }

    void SetPosition(Fixed16_16 x, Fixed16_16 y)
    {
        Position.X = x;
        Position.Y = y;
        UpdateView();
    }

    void SetX(Fixed16_16 x)
    {
        Position.X = x;
        UpdateView();
    }

    void SetY(Fixed16_16 y)
    {
        Position.Y = y;
        UpdateView();
    }

    void Move(Vector2 amount)
    {
        Position += amount;
        UpdateView();
    }

    void Move(Fixed16_16 x, Fixed16_16 y)
    {
        Position.X += x;
        Position.Y += y;
        UpdateView();
    }

    void SetZoom(Fixed16_16 newZoomLevel)
    {
        ZoomLevel = newZoomLevel;
        UpdateView();
    }

    void SetZoom(Fixed16_16 newZoomLevel, Fixed16_16 minZoom, Fixed16_16 maxZoom)
    {
        ZoomLevel = newZoomLevel;

        if (ZoomLevel < minZoom)
        {
            ZoomLevel = minZoom;
        }
        else if (ZoomLevel > maxZoom)
        {
            ZoomLevel = maxZoom;
        }

        UpdateView();
    }

    void Zoom(Fixed16_16 deltaZoom)
    {
        ZoomLevel += deltaZoom;
        UpdateView();
    }

    void Zoom(Fixed16_16 deltaZoom, Fixed16_16 minZoom, Fixed16_16 maxZoom)
    {
        ZoomLevel += deltaZoom;

        if (ZoomLevel < minZoom)
        {
            ZoomLevel = minZoom;
        }
        else if (ZoomLevel > maxZoom)
        {
            ZoomLevel = maxZoom;
        }

        UpdateView();
    }

private:
    constexpr void UpdateView()
    {
        //Calculate the boundaries based on position and zoom
        Fixed16_16 halfWidth = Width / Fixed16_16(2) / ZoomLevel;
        Fixed16_16 halfHeight = Height / Fixed16_16(2) / ZoomLevel;

        //Horizontal axis is flipped?
        Left = -Position.X - halfWidth;
        Right = -Position.X + halfWidth;
        Bottom = Position.Y - halfHeight;
        Top = Position.Y + halfHeight;

        //Cache values
        ScaleX = Width / (Right - Left);
        ScaleY = Height / (Bottom - Top);
    }
};