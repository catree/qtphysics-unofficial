#ifndef WORLD_H
#define WORLD_H

#include <physics_entities/physicsabstractdynamicsworld.h>

#include "../bodies/abstractbody.h"

#include <QVector3D>
#include <btBulletDynamicsCommon.h>

namespace Physics {

namespace Bullet{


class World : public PhysicsAbstractDynamicsWorld
{
    Q_OBJECT

public:
    World(QObject *parent=0);
    ~World();

    WorldType type(){return m_type;}
    void setType(WorldType type);

    qreal simulationRate(){return m_simulationRate;}
    void setSimulationRate(qreal rate);

    void stepSimulation();

    QVector3D gravity(){return m_gravity;}
    void setGravity(QVector3D gravity);

    void removeBody(PhysicsAbstractRigidBody* b);
    void addBody(PhysicsAbstractRigidBody*b);

    void setDebug(bool debug);
    bool debug(){return m_debug;}

    QVector<Collision> getCollisions();


private slots:
    void onBodyDestroyed(QObject* obj);
    void onBodyRequireUpdate();
private:
    void removebtRigidBody(btRigidBody* b);
    void addbtRigidBody(btRigidBody* b,int group,int mask);

    void init();

    WorldType m_type;
    qreal m_simulationRate;
    QVector3D m_gravity;

    QSet<AbstractBody*> m_bodies;

    QHash<AbstractBody*,btCollisionObject*> m_PhysicsBodies2BulletBodies;
    QHash<btCollisionObject*,AbstractBody*> m_BulletBodies2PhysicsBodies;

    btBroadphaseInterface* m_broadphase;
    btDefaultCollisionConfiguration* m_collisionConfiguration;
    btCollisionDispatcher* m_dispatcher;
    btSequentialImpulseConstraintSolver* m_solver;
    btDynamicsWorld* m_dynamicsWorld;

    bool m_debug;

    //the value is the life time: 0 to be removed,1 means old, 2 means new
    QHash<Collision,ushort> m_collitions;

};

}}

#endif // WORLD_H
