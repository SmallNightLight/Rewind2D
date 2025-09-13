#pragma once

#include "../../ECS/ECS.h"
#include "../../Rendering/Camera.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_pixels.h>

class BoxColliderRenderer
{
public:
    using RequiredComponents = ComponentList<Transform, BoxCollider, ColliderRenderData>;

    explicit BoxColliderRenderer(PhysicsComponentManager& componentManager)
    {
        transformCollection = componentManager.GetComponentCollection<Transform>();
        transformMetaCollection = componentManager.GetComponentCollection<TransformMeta>();
        boxColliderCollection = componentManager.GetComponentCollection<BoxCollider>();
        colliderRenderDataCollection = componentManager.GetComponentCollection<ColliderRenderData>();

        Entities.Initialize();
    }

    void Render(SDL_Renderer* renderer, Camera& camera ) const
    {
        for (const Entity& entity : Entities) //todo replace with rect
        {
            Transform& transform = transformCollection->GetComponent(entity);
            BoxCollider& boxCollider = boxColliderCollection->GetComponent(entity);
            ColliderRenderData& colliderRenderData = colliderRenderDataCollection->GetComponent(entity);

            SDL_Color color = {colliderRenderData.R, colliderRenderData.G, colliderRenderData.B, SDL_ALPHA_OPAQUE};
            SDL_FColor colorF = ToFColor(color);    //todo only do once when creation

            //Draw filled box
            Vector2Span vertices = boxCollider.GetTransformedVertices(transform);
            std::array<SDL_Vertex, 4> sdlVertices { };

            for (uint8_t i = 0; i < 4; i++)
            {
                SDL_Vertex v;
                Vector2 screenPos = camera.WorldToScreen(vertices[i]);
                v.position.x = screenPos.X.ToFloating<float>();
                v.position.y = screenPos.Y.ToFloating<float>();
                v.color = colorF;
                v.tex_coord = {0, 0};
                sdlVertices[i] = v;
            }

            int indices[] = {0, 1, 2, 0, 2, 3};
            SDL_RenderGeometry(renderer, nullptr, sdlVertices.data(), 4, indices, 6);

            //Draw white outline
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
            for (size_t i = 0; i < sdlVertices.size(); i++)
            {
                size_t j = (i + 1) % sdlVertices.size();
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
        //     BoxCollider& boxCollider = boxColliderCollection->GetComponent(entity);
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
        //     AABB boundingBox = boxCollider.GetAABB(transform, transformMeta);
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
    ComponentCollection<BoxCollider>* boxColliderCollection;
    ComponentCollection<ColliderRenderData>* colliderRenderDataCollection;

public:
    EntitySet<MAXENTITIES> Entities;
};
