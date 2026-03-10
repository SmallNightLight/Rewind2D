#pragma once

#include "../../ECS/ECS.h"
#include "../PhysicsSettings.h"
#include "../Collision/CollisionDetection.h"

#include <vector>

#include "../Additional/IslandManager.h"
//#include "../Cache/ContactCache2.h"

class RigidBody
{
public:
    using RequiredComponents = ComponentList<Transform, TransformMeta, RigidBodyData>;

    explicit RigidBody(PhysicsComponentManager& componentManager) : //TODO: Static objects should not need to have a rigidBody
        transformCollection(componentManager.GetComponentCollection<Transform>()),
        transformMetaCollection(componentManager.GetComponentCollection<TransformMeta>()),
        rigidBodyDataCollection(componentManager.GetComponentCollection<RigidBodyData>()),
        circleColliderCollection(componentManager.GetComponentCollection<CircleCollider>()),
        boxColliderCollection(componentManager.GetComponentCollection<BoxCollider>()),
        polygonColliderCollection(componentManager.GetComponentCollection<PolygonCollider>()),
        collisionDetection(componentManager), useCache(false), collisionCache(nullptr), physicsCache(nullptr)
    {
        Entities.Initialize();
    }

    void InitializeCache(CollisionCache* pCollisionCache, PhysicsCache* pPhysicsCache)
    {
        collisionCache = pCollisionCache;
        physicsCache = pPhysicsCache;
    }

    void HandleCollisions(FrameNumber frame)
    {
        assert(collisionCache && "CollisionCache is null");

        //Update & Validate collision cache
        useCache = collisionCache->UpdateFrame(frame);

        ContactPairs.clear();
        collisionCache->Flip();

        //Setup transform from cache
        SetupEntityTransforms(useCache);
        collisionCache->CacheTransformCollection(transformCollection);

        for (auto it1 = Entities.begin(); it1 != Entities.end(); ++it1)
        {
            const Entity& entity1 = *it1;
            Transform& transform1 = transformCollection.GetComponent(entity1);

            //Detect collisions
            for (auto it2 = std::next(it1); it2 != Entities.end(); ++it2)
            {
                const Entity& entity2 = *it2;

                //todo Entity pairs should be ordered (currently is)

                Transform& transform2 = transformCollection.GetComponent(entity2);
                EntityPair entityPair = EntityPair::Make(entity1, entity2);
                //std::cout << entityPair.Key << std::endl;
                //Check if collision already occurred in the past
                if (useCache && !transform1.Changed && !transform2.Changed)
                {
                    //The collision has already happened before (same position and rotation)
                    bool collision = collisionCache->AdvancePairCache(entityPair);

                    if (!collision) continue; //todo might be faster to skip this completely

                    //Get collision response data
                    ContactPair cachedContactPair;
                    if (collisionCache->AdvanceCollisionCache(entityPair, cachedContactPair))
                    {
                        ContactPairs.emplace_back(cachedContactPair);
                    }

                    continue;
                }

                RigidBodyData& rigidBodyData1 = rigidBodyDataCollection.GetComponent(entity1);
                RigidBodyData& rigidBodyData2 = rigidBodyDataCollection.GetComponent(entity2);
                TransformMeta& transformMeta1 = transformMetaCollection.GetComponent(entity1);
                TransformMeta& transformMeta2 = transformMetaCollection.GetComponent(entity2);

                ContactPair contactPair = ContactPair();    //Value initialization to give the impulses zero values
                if (collisionDetection.DetectCollision(entity1, entity2, transform1, transform2, transformMeta1, transformMeta2, contactPair))
                {
                    ContactPairs.emplace_back(contactPair);
                    collisionCache->CacheCollisionPair(entityPair);
                    collisionCache->CacheCollision(contactPair);
                }

                //In case of no collision, it is not cached as the fact that it is not present in the cache means that there is not collision.
                //Only possible since we first check if the entity has changed during rollback
            }
        }
    }

    void SetupEntityTransforms(bool useCache) //optimize inline in the handlecol? todo divide into two bools for both
    {
        if (useCache)
        {
            Transform cachedTransform;
            for (const Entity& entity : Entities)
            {
                if (collisionCache->TryGetTransform(entity, cachedTransform))
                {
                    Transform& transform = transformCollection.GetComponent(entity);
                    transform.Changed = transform.Key != cachedTransform.Key;
                }
                else
                {
                    transformCollection.GetComponent(entity).Changed = true;
                }
            }
        }
        else
        {
            for (const Entity& entity : Entities)
            {
                transformCollection.GetComponent(entity).Changed = true;
            }
        }
    }

    void IntegrateForces(Fixed16_16 deltaTime)
    {
        for (const Entity& entity : Entities)
        {
            TransformMeta& transformMeta = transformMetaCollection.GetComponent(entity);

            if (transformMeta.IsStatic) continue;

            RigidBodyData& rigidBodyData = rigidBodyDataCollection.GetComponent(entity);

            rigidBodyData.Base.Velocity += (Gravity + rigidBodyData.Force * rigidBodyData.InverseMass) * deltaTime;
            //rigidBodyData.AngularVelocity += deltaTime * rigidBodyData.InverseInertia * rigidBodyData.Torque; //todo
        }
    }

    static constexpr bool WarmStarting = true;

    void SetupContacts()
    {
        std::vector<EntityPair> entityPairs;
        entityPairs.reserve(ContactPairs.size());

        //Rebuild islands
        for (ContactPair& pair : ContactPairs)
        {
            if (pair.EntityStatic1 || pair.EntityStatic2) continue;

            entityPairs.push_back(pair.EntityKey);
        }

        entityPairs.shrink_to_fit();

        EntityIslandData<true> islandData;
        m_IslandManager.UpdateIslands<true>(entityPairs, islandData);

        //Check if rigidBody was changed
        if (useCache)
        {
            RigidBodyData cachedRigidBodyData;
            for (const Entity& entity : Entities)
            {
                if (collisionCache->TryGetRigidBodyData(entity, cachedRigidBodyData))
                {
                    RigidBodyData& rigidBodyData = rigidBodyDataCollection.GetComponent(entity);
                    rigidBodyData.Changed = rigidBodyData.Key != cachedRigidBodyData.Key;
                }
                else
                {
                    rigidBodyDataCollection.GetComponent(entity).Changed = true;
                }
            }
        }
        else
        {
            for (const Entity& entity : Entities)
            {
                transformCollection.GetComponent(entity).Changed = true;
            }
        }

        //Apply previous impulses
        if (WarmStarting)
        {
            for (ContactPair& contactPair : ContactPairs)
            {
                ImpulseData lastImpulseData;
                if (physicsCache->TryGetImpulseData(contactPair.EntityKey, lastImpulseData))
                {
                    for (uint8_t i = 0; i < contactPair.ContactCount; ++i)
                    {
                        Contact& newContact = contactPair.Contacts[i];
                        for (uint8_t j = 0; j < lastImpulseData.ContactCount; ++j)
                        {
                            if (newContact.LastImpulse.Feature.Value == lastImpulseData.LastImpulses[j].Feature.Value)
                            {
                                //Warm starting
                                newContact.LastImpulse = lastImpulseData.LastImpulses[j];
                                break;
                            }
                        }
                    }
                }
            }
        }

        //Update cache
        collisionCache->CacheRigidBodyDataCollection(rigidBodyDataCollection);

        if constexpr (WarmStarting)
        {
            for (ContactPair& contactPair : ContactPairs)
            {
                RigidBodyData& rigidBodyData1 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity1());
                RigidBodyData& rigidBodyData2 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity2());

                for (int i = 0; i < contactPair.ContactCount; ++i)
                {
                    Contact& contact = contactPair.Contacts[i];
                    Vector2 tangent = contactPair.Normal.Perpendicular();

                    //Apply normal + friction impulse
                    Vector2 P = contactPair.Normal * contact.LastImpulse.Pn + tangent * contact.LastImpulse.Pt;

                    rigidBodyData1.Base.Velocity -= P * rigidBodyData1.InverseMass;
                    rigidBodyData1.Base.AngularVelocity -= rigidBodyData1.InverseInertia * contact.R1.Cross(P);

                    rigidBodyData2.Base.Velocity += P * rigidBodyData2.InverseMass;
                    rigidBodyData2.Base.AngularVelocity += rigidBodyData2.InverseInertia * contact.R2.Cross(P);


                    //Compute MassNormal
                    Fixed16_16 rn1 = contact.R1.Dot(contactPair.Normal);
                    Fixed16_16 rn2 = contact.R2.Dot(contactPair.Normal);
                    Fixed16_16 kNormal = rigidBodyData1.InverseMass + rigidBodyData2.InverseMass;
                    kNormal += rigidBodyData1.InverseInertia * (contact.R1.Dot(contact.R1) - rn1 * rn1) + rigidBodyData2.InverseInertia * (contact.R2.Dot(contact.R2) - rn2 * rn2);
                    contact.MassNormal = Fixed16_16(1) / kNormal;

                    //Compute MassTangent
                    Fixed16_16 rt1 = contact.R1.Dot(tangent);
                    Fixed16_16 rt2 = contact.R2.Dot(tangent);
                    Fixed16_16 kTangent = rigidBodyData1.InverseMass + rigidBodyData2.InverseMass;
                    kTangent += rigidBodyData1.InverseInertia * (contact.R1.Dot(contact.R1) - rt1 * rt1) + rigidBodyData2.InverseInertia * (contact.R2.Dot(contact.R2) - rt2 * rt2);
                    contact.MassTangent = Fixed16_16(1) / kTangent;
                }
            }
        }
        else
        {
            for (ContactPair& contactPair : ContactPairs)
            {
                RigidBodyData& rigidBodyData1 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity1());
                RigidBodyData& rigidBodyData2 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity2());

                //Precompute tangents on all contacts
                for (int i = 0; i < contactPair.ContactCount; ++i)
                {
                    Contact& contact = contactPair.Contacts[i];

                    //Compute MassNormal
                    Fixed16_16 rn1 = contact.R1.Dot(contactPair.Normal);
                    Fixed16_16 rn2 = contact.R2.Dot(contactPair.Normal);
                    Fixed16_16 kNormal = rigidBodyData1.InverseMass + rigidBodyData2.InverseMass;
                    kNormal += rigidBodyData1.InverseInertia * (contact.R1.Dot(contact.R1) - rn1 * rn1) + rigidBodyData2.InverseInertia * (contact.R2.Dot(contact.R2) - rn2 * rn2);
                    contact.MassNormal = Fixed16_16(1) / kNormal;

                    //Compute MassTangent
                    Vector2 tangent = contactPair.Normal.Perpendicular();
                    Fixed16_16 rt1 = contact.R1.Dot(tangent);
                    Fixed16_16 rt2 = contact.R2.Dot(tangent);
                    Fixed16_16 kTangent = rigidBodyData1.InverseMass + rigidBodyData2.InverseMass;
                    kTangent += rigidBodyData1.InverseInertia * (contact.R1.Dot(contact.R1) - rt1 * rt1) + rigidBodyData2.InverseInertia * (contact.R2.Dot(contact.R2) - rt2 * rt2);
                    contact.MassTangent = Fixed16_16(1) / kTangent;
                }
            }
        }
    }

    void SolveContacts(Fixed16_16 deltaTime)
    {
        for (ContactPair& contactPair : ContactPairs)
        {
            RigidBodyData& rigidBodyData1 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity1());
            RigidBodyData& rigidBodyData2 = rigidBodyDataCollection.GetComponent(contactPair.EntityKey.Entity2());

            for (int i = 0; i < contactPair.ContactCount; ++i)
            {
                Contact& contact = contactPair.Contacts[i];

                //Relative velocity at contact
                Vector2 dv = rigidBodyData2.Base.Velocity + contact.R2.CrossI(rigidBodyData2.Base.AngularVelocity) - rigidBodyData1.Base.Velocity - contact.R1.CrossI(rigidBodyData1.Base.AngularVelocity);

                //Compute normal impulse
                Fixed16_16 vn = dv.Dot(contactPair.Normal);

                // NEW
                constexpr Fixed16_16 linearSlop = Fixed16_16(1) / Fixed16_16(200);          // ~0.005
                constexpr Fixed16_16 baumgarteBeta = Fixed16_16(1) / Fixed16_16(3);         // 0.33

                Fixed16_16 dPn;

                if (contact.Separation > Fixed16_16(0))
                {
                    //Speculative contact
                    Fixed16_16 predictiveBias = contact.Separation / deltaTime;
                    dPn = contact.MassNormal * -(vn + predictiveBias);
                }
                else if (contact.Separation > -linearSlop)
                {
                    Fixed16_16 penetration = -contact.Separation;
                    constexpr Fixed16_16 maxCorrection = Fixed16_16(1) / Fixed16_16(60);
                    Fixed16_16 positionalError = penetration - linearSlop;
                    //positionalError = fpm::min(positionalError, maxCorrection);
                    Fixed16_16 bias = (baumgarteBeta / deltaTime) * positionalError;
                    dPn = contact.MassNormal * (-vn + bias);
                }
                else
                {
                    dPn = contact.MassNormal * -vn;
                }

                if constexpr (WarmStarting)
                {
                    //Clamp the accumulated impulse
                    Fixed16_16 Pn0 = contact.LastImpulse.Pn;
                    contact.LastImpulse.Pn = fpm::max(Pn0 + dPn, Fixed16_16(0));
                    dPn = contact.LastImpulse.Pn - Pn0;
                }
                else
                {
                    dPn = fpm::max(dPn, Fixed16_16(0));
                }

                //Apply contact impulse
                Vector2 Pn = contactPair.Normal * dPn;

                rigidBodyData1.Base.Velocity -= Pn * rigidBodyData1.InverseMass;
                rigidBodyData1.Base.AngularVelocity -= rigidBodyData1.InverseInertia * contact.R1.Cross(Pn);

                rigidBodyData2.Base.Velocity += Pn * rigidBodyData2.InverseMass;
                rigidBodyData2.Base.AngularVelocity += rigidBodyData2.InverseInertia * contact.R2.Cross(Pn);

                //Relative velocity at contact
                dv = rigidBodyData2.Base.Velocity + contact.R2.CrossI(rigidBodyData2.Base.AngularVelocity) - rigidBodyData1.Base.Velocity - contact.R1.CrossI(rigidBodyData1.Base.AngularVelocity);

                Vector2 tangent = contactPair.Normal.Perpendicular();
                Fixed16_16 vt = dv.Dot(tangent);
                Fixed16_16 dPt = contact.MassTangent * -vt;

                if constexpr (WarmStarting)
                {
                    //Compute friction impulse
                    Fixed16_16 maxPt = contactPair.Friction *  contact.LastImpulse.Pn;

                    //Clamp friction
                    Fixed16_16 oldTangentImpulse =  contact.LastImpulse.Pt;
                    contact.LastImpulse.Pt = Clamp(oldTangentImpulse + dPt, -maxPt, maxPt);
                    dPt =  contact.LastImpulse.Pt - oldTangentImpulse;
                }
                else
                {
                    Fixed16_16 maxPt = contactPair.Friction * dPn;
                    dPt = Clamp(dPt, -maxPt, maxPt);
                }

                //Apply contact impulse
                Vector2 Pt = tangent * dPt;

                rigidBodyData1.Base.Velocity -= Pt * rigidBodyData1.InverseMass;
                rigidBodyData1.Base.AngularVelocity -= rigidBodyData1.InverseInertia * contact.R1.Cross(Pt);

                rigidBodyData2.Base.Velocity += Pt * rigidBodyData2.InverseMass;
                rigidBodyData2.Base.AngularVelocity += rigidBodyData2.InverseInertia * contact.R2.Cross(Pt);
            }
        }
    }

    void IntegrateVelocities(Fixed16_16 deltaTime)
    {
        for (const Entity& entity : Entities)
        {
            TransformMeta& transformMeta = transformMetaCollection.GetComponent(entity);

            if (transformMeta.IsStatic) continue;

            Transform& transform = transformCollection.GetComponent(entity);
            RigidBodyData& rigidBodyData = rigidBodyDataCollection.GetComponent(entity);

            // Apply global damping todo
            //constexpr Fixed16_16 linearDamping = Fixed16_16(1) / Fixed16_16(100);
            //constexpr Fixed16_16 angularDamping = Fixed16_16(1) / Fixed16_16(100);
            //rigidBodyData.Base.Velocity *= 1 - linearDamping;
            //rigidBodyData.Base.AngularVelocity *= 1 - angularDamping;

            transform.MovePosition(rigidBodyData.Base.Velocity * deltaTime);
            transform.Rotate(rigidBodyData.Base.AngularVelocity * deltaTime);

            rigidBodyData.Force = Vector2(0, 0);
            //rigidBodyData.Torque = Fixed16_16(0); //todo
        }
    }

    void IntegratePositions()
    {
        physicsCache->ResetImpulses();

        for (ContactPair& contactPair : ContactPairs)
        {
            Entity entity1 = contactPair.EntityKey.Entity1();
            Entity entity2 = contactPair.EntityKey.Entity2();

            TransformMeta& transformMeta1 = transformMetaCollection.GetComponent(entity1);     //todo the static check is not worth the access also above
            TransformMeta& transformMeta2 = transformMetaCollection.GetComponent(entity2);
            Transform& transform1 = transformCollection.GetComponent(entity1);
            Transform& transform2 = transformCollection.GetComponent(entity2);
            RigidBodyData& rigidBodyData1 = rigidBodyDataCollection.GetComponent(entity1);
            RigidBodyData& rigidBodyData2 = rigidBodyDataCollection.GetComponent(entity2);

            for (uint8_t i = 0; i < contactPair.ContactCount; ++i)
            {
                Contact& contact = contactPair.Contacts[i];

                //Position correction
                constexpr Fixed16_16 steeringConstant = Fixed16_16(0, 5);
                constexpr Fixed16_16 maxCorrection = -Fixed16_16(5);
                constexpr Fixed16_16 slop = Fixed16_16(1) / Fixed16_16(100);

                Fixed16_16 steeringForce = Clamp(steeringConstant * (contact.Separation + slop), maxCorrection, Fixed16_16(0));
                Vector2 impulse = contactPair.Normal * (-steeringForce * contact.MassNormal);

                if (!transformMeta1.IsStatic)
                {
                    transform1.MovePosition(-impulse * rigidBodyData1.InverseMass);
                    Vector2 r1 = contact.Position - transform1.Base.Position;
                    transform1.Rotate(-r1.Cross(impulse) * rigidBodyData1.InverseInertia);
                }

                if (!transformMeta2.IsStatic)
                {
                    transform2.MovePosition(impulse * rigidBodyData2.InverseMass);
                    Vector2 r2 = contact.Position - transform2.Base.Position;
                    transform2.Rotate(r2.Cross(impulse) * rigidBodyData2.InverseInertia);
                }
            }

            //Cache impulses
            ImpulseData newImpulses;
            newImpulses.EntityKey = contactPair.EntityKey;
            newImpulses.ContactCount = contactPair.ContactCount;

            for (uint8_t i = 0; i < contactPair.ContactCount; ++i)
            {
                newImpulses.LastImpulses[i] = contactPair.Contacts[i].LastImpulse;
            }

            physicsCache->CacheImpulseData(newImpulses);
        }
    }

private:
    inline static Fixed16_16 Clamp(Fixed16_16 value, Fixed16_16 min, Fixed16_16 max)
    {
        return fpm::max(min, fpm::min(value, max));
    }

private:
    ComponentCollection<Transform>& transformCollection;
    ComponentCollection<TransformMeta>& transformMetaCollection;
    ComponentCollection<RigidBodyData>& rigidBodyDataCollection;
    ComponentCollection<CircleCollider>& circleColliderCollection;
    ComponentCollection<BoxCollider>& boxColliderCollection;
    ComponentCollection<PolygonCollider>& polygonColliderCollection;

    CollisionDetection collisionDetection;

    //Caching
    CollisionCache* collisionCache;
    PhysicsCache* physicsCache;

    bool useCache;

    IslandManager m_IslandManager { };

public:
    std::vector<ContactPair> ContactPairs;
    EntitySet Entities;
};