#include "updatephysicsentitiesjob.h"
#include "backendtypes/physicsentity.h"
#include "backendtypes/physicsgeometryrenderer.h"
#include "backendtypes/physicsbodyinfobackendnode.h"
#include "backendtypes/physicsworldinfobackendnode.h"
#include "backendtypes/physicstransform.h"
#include "backendtypes/physicsgeometry.h"
#include "backendtypes/physicsattribute.h"
#include "backendtypes/physicsbuffer.h"

#include "physicsmanager.h"
namespace Physics {


UpdatePhysicsEntitiesJob::UpdatePhysicsEntitiesJob(PhysicsManager* manager):
    Qt3D::QAspectJob()
{
    m_manager=manager;
}

void UpdatePhysicsEntitiesJob::run(){
    recursive_step(m_manager->rootEntityId(),QMatrix4x4(),false);
    Q_FOREACH(Qt3D::QNodeId id,m_manager->garbage){
        if(m_manager->m_Id2RigidBodies.contains(id)){
            PhysicsAbstractRigidBody* body =m_manager->m_Id2RigidBodies[id];
            m_manager->m_Id2RigidBodies.remove(id);
            m_manager->m_RigidBodies2Id.remove(body);
            m_manager->m_physics_world->removeRigidBody(body);
            delete body;
        }
    }
    m_manager->garbage.clear();
}

void UpdatePhysicsEntitiesJob::recursive_step(Qt3D::QNodeId node_id, QMatrix4x4 parent_matrix,bool forceUpdateMS){
    if(node_id.isNull()) return;
    PhysicsEntity* entity= static_cast<PhysicsEntity*>(m_manager->m_resources.operator [](node_id));
    QMatrix4x4 current_global_matrix=parent_matrix;
    PhysicsBodyInfoBackendNode* entity_body_info=Q_NULLPTR;
    if(!entity->physicsBodyInfo().isNull())
        entity_body_info=static_cast<PhysicsBodyInfoBackendNode*>(m_manager->m_resources.operator [](entity->physicsBodyInfo()));
    else {
        if(m_manager->m_Id2RigidBodies.contains(node_id)){
            m_manager->garbage.insert(node_id);
        }
    }
    bool isBodyNew=false;
    PhysicsAbstractRigidBody* rigid_body=retrievePhysicalBody(entity,entity_body_info,isBodyNew);
    if(rigid_body){
        /*Update collision Shape*/
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::ShapeDetailsChanged)){
            //TODO
        }
        //else if(entity_geometry_renderer!=Q_NULLPTR && entity_geometry_renderer->isDirty()){
        //TODO
        //}
        /*Update Motion State*/
        PhysicsTransform* inputTransform=Q_NULLPTR;
        if(!entity_body_info->inputTransform().isNull()){
            inputTransform=static_cast<PhysicsTransform*>(m_manager->m_resources.operator [](entity_body_info->inputTransform()));
        }
        //qDebug()<<"Bah";
        //qDebug()<<current_global_matrix;
        /*The input transform has changed; the new position is taken from that.*/
  //     qDebug()<<current_global_matrix;
        if(inputTransform!=Q_NULLPTR && (inputTransform->isDirty() || isBodyNew )){
            current_global_matrix=current_global_matrix*inputTransform->transformMatrix();
            forceUpdateMS=true;
            inputTransform->setDirty(false);
        }
        /*Otherwise use the matrix form the simulation*/
        else{
//            qDebug()<<"Inside";
            current_global_matrix=current_global_matrix*entity_body_info->localTransform();
            //rigid_body->worldTransformation();
        }
//        qDebug()<<current_global_matrix;
        if(forceUpdateMS){
            rigid_body->setWorldTransformation(current_global_matrix);
        }
        /*Update Body properties*/
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::MaskChanged)){
            rigid_body->setMask(entity_body_info->mask());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::MaskChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::GroupChanged)){
            rigid_body->setGroup(entity_body_info->group());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::GroupChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::MassChanged)){
            rigid_body->setMass(entity_body_info->mass());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::MassChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::KinematicChanged)){
            rigid_body->setKinematic(entity_body_info->kinematic());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::KinematicChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::FallInertiaChanged)){
            rigid_body->setFallInertia(entity_body_info->fallInertia());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::FallInertiaChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::RestistutionChanged)){
            rigid_body->setRestitution(entity_body_info->restitution());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::RestistutionChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::FrictionChanged)){
            rigid_body->setFriction(entity_body_info->friction());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::FrictionChanged;
        }
        if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::RollingFrictionChanged)){
            rigid_body->setRollingFriction(entity_body_info->rollingFriction());
            entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::RollingFrictionChanged;
        }
    }
    else{
        if(!entity->transform().isNull()){
            /*Entity with only a transformation*/
            PhysicsTransform* entity_transform=static_cast<PhysicsTransform*>(m_manager->m_resources.operator [](entity->transform()));
            current_global_matrix=current_global_matrix*entity_transform->transformMatrix();
            if(entity_transform->isDirty()){
                forceUpdateMS=true;
                entity_transform->setDirty(false);
            }
        }
    }

    /*Update Physic World settings*/
    if(!entity->physicsWorldInfo().isNull()){
        PhysicsWorldInfoBackendNode* entity_physics_world_info=static_cast<PhysicsWorldInfoBackendNode*>(m_manager->m_resources.operator [](entity->physicsWorldInfo()));
        if(entity_physics_world_info->dirtyFlags().testFlag(PhysicsWorldInfoBackendNode::DirtyFlag::GravityChanged)){
            m_manager->m_physics_world->setGravity(entity_physics_world_info->gravity());
            entity_physics_world_info->dirtyFlags() &= ~PhysicsWorldInfoBackendNode::DirtyFlag::GravityChanged;
            m_manager->m_physics_world->setDebug(entity_physics_world_info->debug());
        }
    }

    /*Next call*/
    for(Qt3D::QNodeId childId : entity->childrenIds())
        recursive_step(childId, current_global_matrix,forceUpdateMS);
}

PhysicsAbstractRigidBody* UpdatePhysicsEntitiesJob::retrievePhysicalBody(PhysicsEntity* entity,PhysicsBodyInfoBackendNode* entity_body_info, bool& isBodyNew){
    isBodyNew=false;
    if(m_manager->m_Id2RigidBodies.contains(entity->peerUuid()))
        return m_manager->m_Id2RigidBodies[entity->peerUuid()];
   /*The entity has a component body info and either a abstract mesh or a the shape details to define the collision shape */
   if(entity_body_info!=Q_NULLPTR
               && (entity_body_info->shapeDetails().size()>0 || !entity->geometry_renderer().isNull())){
        PhysicsGeometryRenderer* entity_geometry_renderer=Q_NULLPTR;
        PhysicsAbstractRigidBody* rigid_body;
        if(entity_body_info->shapeDetails().size()>0){
            rigid_body=createRigidBodyFromShapeDetails(entity_body_info);
        }
        else if(!entity->geometry_renderer().isNull()){
            entity_geometry_renderer=static_cast<PhysicsGeometryRenderer*>(m_manager->m_resources.operator [](entity->geometry_renderer()));
            rigid_body=createRigidBodyFromMesh(entity_geometry_renderer);
        }
        if(rigid_body){
            m_manager->m_Id2RigidBodies[entity->peerUuid()]=rigid_body;
            m_manager->m_RigidBodies2Id[rigid_body]=entity->peerUuid();
            m_manager->m_physics_world->addRigidBody(rigid_body);
            isBodyNew=true;
        }
        return rigid_body;
    }
    return Q_NULLPTR;
}

PhysicsAbstractRigidBody* UpdatePhysicsEntitiesJob::createRigidBodyFromMesh(PhysicsGeometryRenderer* entity_geometry_renderer){
    QVariantMap geometric_info;
    if(entity_geometry_renderer->m_type==PhysicsGeometryRenderer::Mesh_Type::CUBOID){
        geometric_info["Type"]="Cuboid";
        geometric_info["X_Dim"]=entity_geometry_renderer->m_x_dim;
        geometric_info["Y_Dim"]=entity_geometry_renderer->m_y_dim;
        geometric_info["Z_Dim"]=entity_geometry_renderer->m_z_dim;
    }
    else if(entity_geometry_renderer->m_type==PhysicsGeometryRenderer::Mesh_Type::SPHERE){
        geometric_info["Type"]="Sphere";
        geometric_info["Radius"]=entity_geometry_renderer->m_radius;
    }
    else{
        geometric_info["Type"]="Generic";
        Qt3D::QNodeId geometryId;
        if(!entity_geometry_renderer->m_geometry.isNull()){
            geometryId=entity_geometry_renderer->m_geometry;
        }
        else{
            geometryId=entity_geometry_renderer->m_geometry_functor.data()->operator ()()->id();
        }       
        PhysicsGeometry* geometry=static_cast<PhysicsGeometry*>(m_manager->m_resources.operator [](geometryId));
        if(!geometry) return Q_NULLPTR;
        QVector<QVector3D> vertexPosition;
        QSet<quint16> index_Set;
        Q_FOREACH(Qt3D::QNodeId attrId,geometry->attributes()){
            PhysicsAttribute* attribute=static_cast<PhysicsAttribute*>(m_manager->m_resources.operator [](attrId));
            if(!attribute) continue;
            if(attribute->objectName()=="vertexPosition"){
                vertexPosition=attribute->asVector3D();             
            }
            else if(attribute->attributeType()==Qt3D::QAttribute::IndexAttribute){
                if(!attribute->bufferId().isNull() && m_manager->m_resources.contains(attribute->bufferId())){
                    QByteArray buffer;
                    PhysicsBuffer* buffer_node=static_cast<PhysicsBuffer*>(m_manager->m_resources.operator [](attribute->bufferId()));
                    if(!buffer_node) continue;
                    if(buffer_node->bufferFunctor().isNull()){
                        buffer=buffer_node->data();
                    }
                    else{
                        buffer=buffer_node->bufferFunctor().data()->operator ()();
                    }
                    const char *rawBuffer = buffer.constData();
                    rawBuffer += attribute->byteOffset();
                    const quint16* fptr;
                    int stride;
                    switch (attribute->dataType()) {
                    case Qt3D::QAttribute::UnsignedShort:
                        stride = sizeof(quint16) * attribute->dataSize();
                        break;
                    default:
                        qDebug()<< "can't convert";
                    }
                    if (attribute->byteStride() != 0)
                        stride = attribute->byteStride();
                    index_Set.reserve(attribute->count());
                    for (uint c = 0; c < attribute->count(); ++c) {
                        fptr = reinterpret_cast<const quint16*>(rawBuffer);
                        for (uint i = 0, m = qMin(attribute->dataSize(), 3U); i < m; ++i)
                            index_Set.insert(fptr[i]);
                        rawBuffer += stride;
                    }
                }
            }
        }
        QVector<QVector3D> v=vertexPosition;
        vertexPosition.clear();
        vertexPosition.reserve(index_Set.size());
        Q_FOREACH(quint16 index,index_Set.values()){
            vertexPosition.append(v[index]);
        }
        if(vertexPosition.size()==0) return Q_NULLPTR;
        geometric_info["Points"]=QVariant::fromValue(vertexPosition);
    }
    if(entity_geometry_renderer!=Q_NULLPTR && entity_geometry_renderer->isDirty()){
        entity_geometry_renderer->setDirty(false);
    }
    return m_manager->m_physics_factory->create_rigid_body(geometric_info);
}

PhysicsAbstractRigidBody* UpdatePhysicsEntitiesJob::createRigidBodyFromShapeDetails(PhysicsBodyInfoBackendNode* entity_body_info){
    QVariantMap geometric_info;
    QVariantMap shapeDetails=entity_body_info->shapeDetails();
    QString type=shapeDetails["Type"].toString();
    if(type=="Cuboid"){
        geometric_info["Type"]="Cuboid";
        geometric_info["X_Dim"]=shapeDetails["X_Dim"];
        geometric_info["Y_Dim"]=shapeDetails["Y_Dim"];
        geometric_info["Z_Dim"]=shapeDetails["Z_Dim"];
    }
    else if(type=="Sphere"){
        geometric_info["Type"]="Sphere";
        geometric_info["Radius"]=shapeDetails["Radius"];
    }
    else if(type=="StaticPlane"){
        geometric_info["Type"]="StaticPlane";
        geometric_info["PlaneNormal"]=shapeDetails["PlaneNormal"];
        geometric_info["PlaneConstant"]=shapeDetails["PlaneConstant"];
    }
    else{
        qFatal("Unknown shape type");
    }
    if(entity_body_info->dirtyFlags().testFlag(PhysicsBodyInfoBackendNode::DirtyFlag::ShapeDetailsChanged)){
        entity_body_info->dirtyFlags() &= ~PhysicsBodyInfoBackendNode::DirtyFlag::ShapeDetailsChanged;
    }
    return m_manager->m_physics_factory->create_rigid_body(geometric_info);
}


}
