#include "Algorithms/CollisionsResolution.hpp"



namespace Gg {
  namespace Algorithm {

    CollisionsResolution::CollisionsResolution(Gg::GulgEngine &gulgEngine,Gg::Entity &w,Gg::Algorithm::UpdateCollisions* c):
      AbstractAlgorithm{gulgEngine},world{w},collisions(c) {

      m_signature = gulgEngine.getComponentSignature("SceneObject");
      m_signature += gulgEngine.getComponentSignature("Collider");
      m_signature += gulgEngine.getComponentSignature("Forces");

    }
    CollisionsResolution::~CollisionsResolution() {}

    void CollisionsResolution::apply() {
      glm::mat4 wT{std::static_pointer_cast<Gg::Component::SceneObject>(m_gulgEngine.getComponent(world, "SceneObject"))->m_globalTransformations};
      std::shared_ptr<VoxelMap> vM{
        std::static_pointer_cast<VoxelMap>(m_gulgEngine.getComponent(world, "VoxelMap"))
      };
      //For each entity :
    	for(std::pair<Gg::Entity,std::vector<int>> currentEntity: collisions->entity_world_collisions) {
        glm::mat4 eT{std::static_pointer_cast<Gg::Component::SceneObject>(m_gulgEngine.getComponent(currentEntity.first, "SceneObject"))->m_globalTransformations};
        glm::vec3 ePosition{
          eT[3][0],eT[3][1],eT[3][2]
        };
         std::shared_ptr<Gg::Component::Collider> eCollider {
           std::static_pointer_cast<Gg::Component::Collider>(m_gulgEngine.getComponent(currentEntity.first, "Collider"))
         };
         std::shared_ptr<Gg::Component::Forces> eForces {
           std::static_pointer_cast<Gg::Component::Forces>(m_gulgEngine.getComponent(currentEntity.first, "Forces"))
         };
         std::vector<int> voxelToCheck {currentEntity.second};

         ePosition -= 0.5f;

         glm::vec3 brE {ePosition + eCollider->r};
         // brE-=eForces->forces;
         glm::vec3 collisional_response{0.f,0.f,0.f};

         for(unsigned int i{0};i<voxelToCheck.size();i++){
            glm::vec3 posV = vM->getVoxelPosition(voxelToCheck[i]);
            glm::vec3 brV = -1.f *  (posV-0.5f);
            glm::vec3 df {brE-brV};
            if(std::abs(df[0])> std::abs(df[1]) && std::abs(df[0])> std::abs(df[2]) ){
              df[2]=0.f;
              df[1]=0.f;
            }else if(std::abs(df[1])> std::abs(df[0]) && std::abs(df[1])> std::abs(df[2])){
              df[2]=0.f;
              df[0]=0.f;
            } else{
              df[0]=0.f;
              df[1]=0.f;
            }
            df = glm::normalize(df);
            glm::vec3 v{posV -df};
            // std::cout<<to_string(df)
            // <<to_string(eForces->velocity+eForces->forces)
            // <<std::endl;

            if( glm::dot(df,(eForces->velocity+eForces->forces))<0.f
                && v[0]>=0.f && v[1]>=0.f && v[2]>=0.f
                && v[0] < vM->getWorldDimensions()[0] && v[1] < vM->getWorldDimensions()[1]&& v[2] < vM->getWorldDimensions()[2]
                && vM->getColor(v[0],v[1],v[2])[3]<0.2f){
              for(unsigned int i{0};i<3;i++){
                if(std::abs(df[i])>0.f  && std::abs(collisional_response[i]) == 0.f  ){
                    collisional_response[i] +=(eForces->velocity[i]+eForces->forces[i]);
                    i=3;
                  }
                }
              }
            }

          eForces->velocity/=1.1f;

          // std::cout<<to_string(-collisional_response)<<std::endl;
            eForces->addForce(-collisional_response );
      }
      //For each entity :
    	for(std::pair<Gg::Entity,Gg::Entity> collidingEntity: collisions->entity_entity_collisions) {

      }
    }

  }
}