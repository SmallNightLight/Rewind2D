#pragma once

#include "../../ECS/ECS.h"
#include "../../Rendering/Camera.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_pixels.h>

class PolygonColliderRenderer
{
public:
    using RequiredComponents = ComponentList<Transform, PolygonCollider, ColliderRenderData>;

    explicit PolygonColliderRenderer(PhysicsComponentManager& componentManager)
    {
        transformCollection = componentManager.GetComponentCollection<Transform>();
        transformMetaCollection = componentManager.GetComponentCollection<TransformMeta>();
        polygonColliderCollection = componentManager.GetComponentCollection<PolygonCollider>();
        colliderRenderDataCollection = componentManager.GetComponentCollection<ColliderRenderData>();

        Entities.Initialize();
    }

    void Render(SDL_Renderer* renderer, Camera& camera) const
    {
        for (const Entity& entity : Entities)
        {
            Transform& transform = transformCollection->GetComponent(entity);
            PolygonCollider& polygonCollider = polygonColliderCollection->GetComponent(entity);
            ColliderRenderData& colliderRenderData = colliderRenderDataCollection->GetComponent(entity);

            SDL_Color color = {colliderRenderData.R, colliderRenderData.G, colliderRenderData.B, SDL_ALPHA_OPAQUE};
            SDL_FColor colorF = ToFColor(color);    //todo only do once when creation

            //Draw filled box
            Vector2Span vertices = polygonCollider.GetTransformedVertices(transform);
            std::vector<SDL_Vertex> sdlVertices(vertices.size);

            for (uint8_t i = 0; i < vertices.size; i++)
            {
                SDL_Vertex v;
                Vector2 screenPos = camera.WorldToScreen(vertices[i]);
                v.position.x = screenPos.X.ToFloating<float>();
                v.position.y = screenPos.Y.ToFloating<float>();
                v.color = colorF;
                v.tex_coord = {0, 0};
                sdlVertices[i] = v;
            }

            std::vector<int> indices;
            indices.reserve(vertices.size * 3);
            for (size_t i = 1; i + 1 < vertices.size; i++)
            {
                indices.push_back(0);
                indices.push_back(static_cast<int>(i));
                indices.push_back(static_cast<int>(i + 1));
            }

            SDL_RenderGeometry(renderer, nullptr, sdlVertices.data(), static_cast<int>(sdlVertices.size()), indices.data(), static_cast<int>(indices.size()));

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
        //     PolygonCollider& polygonCollider = polygonColliderCollection->GetComponent(entity);
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
        //     AABB boundingBox = polygonCollider.GetAABB(transform, transformMeta);
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
    ComponentCollection<PolygonCollider>* polygonColliderCollection;
    ComponentCollection<ColliderRenderData>* colliderRenderDataCollection;

public:
    EntitySet<MAXENTITIES> Entities;
};