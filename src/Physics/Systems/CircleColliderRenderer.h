#pragma once

#include "../../ECS/ECS.h"
#include "../../Rendering/Camera.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_pixels.h>

class CircleColliderRenderer
{
public:
    using RequiredComponents = ComponentList<Transform, CircleCollider, ColliderRenderData>;

    explicit CircleColliderRenderer(PhysicsComponentManager& componentManager)
    {
        transformCollection = componentManager.GetComponentCollection<Transform>();
        transformMetaCollection = componentManager.GetComponentCollection<TransformMeta>();
        circleColliderCollection = componentManager.GetComponentCollection<CircleCollider>();
        colliderRenderDataCollection = componentManager.GetComponentCollection<ColliderRenderData>();

        Entities.Initialize();
    }

    void Render(SDL_Renderer* renderer, Camera& camera) const
    {
        for (const Entity& entity : Entities)
        {
            Transform& transform = transformCollection->GetComponent(entity);
            CircleCollider& circleCollider = circleColliderCollection->GetComponent(entity);
            ColliderRenderData& colliderRenderData = colliderRenderDataCollection->GetComponent(entity);

            SDL_Color color = {colliderRenderData.R, colliderRenderData.G, colliderRenderData.B, SDL_ALPHA_OPAQUE};
            SDL_FColor colorF = ToFColor(color);    //todo only do once when creation

            auto x = transform.Base.Position.X.ToFloating<float>();
            auto y = transform.Base.Position.Y.ToFloating<float>();
            auto radius = circleCollider.GetRadius().ToFloating<float>();
            int numSegments = 32;

            //Draw filled circle
            std::vector<SDL_Vertex> sdlVertices(numSegments + 1);

            //Add center vertex
            Vector2 circleCenter = camera.WorldToScreen(transform.Base.Position);
            sdlVertices[0].position = { circleCenter.X.ToFloating<float>(), circleCenter.Y.ToFloating<float>() };
            sdlVertices[0].color = colorF;
            sdlVertices[0].tex_coord = {0, 0};

            for (int i = 0; i < numSegments; i++)
            {
                float theta = 2.0f * 3.1415926f * static_cast<float>(i) / static_cast<float>(numSegments);
                float dx = radius * cosf(theta);
                float dy = radius * sinf(theta);
                SDL_FPoint point { x + dx, y + dy };

                sdlVertices[i + 1].position = camera.WorldToScreen(point);
                sdlVertices[i + 1].color = colorF;
                sdlVertices[i + 1].tex_coord = {0, 0};
            }

            //Triangle fan indices
            std::vector<int> indices;
            indices.reserve(numSegments * 3);
            for (int i = 0; i < numSegments; i++)
            {
                indices.push_back(0);
                indices.push_back(i + 1);
                indices.push_back((i + 1) % numSegments + 1);
            }

            SDL_RenderGeometry(renderer, nullptr, sdlVertices.data(), static_cast<int>(sdlVertices.size()), indices.data(), static_cast<int>(indices.size()));

            //Draw white outline
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            for (size_t i = 1; i < sdlVertices.size(); i++)
            {
                size_t j = (i + 1 < sdlVertices.size()) ? i + 1 : 1; // wrap around to first perimeter vertex
                SDL_RenderLine(renderer, sdlVertices[i].position.x, sdlVertices[i].position.y, sdlVertices[j].position.x, sdlVertices[j].position.y);
            }
        }
    }

    void RenderDebugOverlay() const
    {
        // for (const Entity& entity : Entities)
        // {
        //     if (!transformMetaCollection->HasComponent(entity)) continue;
        //
        //     Transform& transform = transformCollection->GetComponent(entity);
        //     TransformMeta& transformMeta = transformMetaCollection->GetComponent(entity);
        //     CircleCollider& circleCollider = circleColliderCollection->GetComponent(entity);
        //
        //     if (transformMeta.Active)
        //     {
        //         glColor3ub(128, 128, 128);
        //     }
        //     else
        //     {
        //         glColor3ub(0, 255, 0);
        //     }
        //
        //     glLineWidth(2.0f);
        //     glBegin(GL_LINE_LOOP);
        //
        //     AABB boundingBox = circleCollider.GetAABB(transform, transformMeta);
        //     glVertex2f(boundingBox.Min.X.ToFloating<float>(), boundingBox.Min.Y.ToFloating<float>());
        //     glVertex2f(boundingBox.Min.X.ToFloating<float>(), boundingBox.Max.Y.ToFloating<float>());
        //     glVertex2f(boundingBox.Max.X.ToFloating<float>(), boundingBox.Max.Y.ToFloating<float>());
        //     glVertex2f(boundingBox.Max.X.ToFloating<float>(), boundingBox.Min.Y.ToFloating<float>());
        //
        //     glEnd();
        // }
    }

    //todo multiple instances of this code
    static SDL_FColor ToFColor(const SDL_Color& color)
    {
        return SDL_FColor
        {
            static_cast<float>(color.r) / 255.0f,
            static_cast<float>(color.g) / 255.0f,
            static_cast<float>(color.b) / 255.0f,
            static_cast<float>(color.a) / 255.0f
        };
    }

private:
    ComponentCollection<Transform>* transformCollection;
    ComponentCollection<TransformMeta>* transformMetaCollection;        //Only for debug todo: remove reference in release build
    ComponentCollection<CircleCollider>* circleColliderCollection;
    ComponentCollection<ColliderRenderData>* colliderRenderDataCollection;

public:
    EntitySet<MAXENTITIES> Entities;
};