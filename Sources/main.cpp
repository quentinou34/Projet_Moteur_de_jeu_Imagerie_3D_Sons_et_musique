#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/vec3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/transform.hpp>

#include <FMOD/fmod_studio.hpp>
#include <FMOD/fmod_errors.h>

#include "GulgEngine/GulgEngine.hpp"

#include "Components/Mesh.hpp"
#include "Components/Transformation.hpp"
#include "Components/SceneObject.hpp"
#include "Components/Light.hpp"
#include "Components/Collider.hpp"
#include "Components/Explosive.hpp"
#include "Components/Timer.hpp"

#include "Systems/UpdateScene.hpp"
#include "Systems/Collisions.hpp"
#include "Systems/Physics.hpp"
#include "Systems/DrawScene.hpp"
#include "Systems/Lightning.hpp"
#include "Systems/Time.hpp"

#include "LoadAnimation.hpp"
#include "NewMap.hpp"
#include "LoadSound.hpp"


bool initOpenGL(GLFWwindow **window) {

    glfwInit();

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); //We don't want the old OpenGL

        // Open a window and create its OpenGL context
    *window = glfwCreateWindow( 800, 600, "C'est le projet de votre année", NULL, NULL);

    if(*window == nullptr){

        std::cout << "ERROR: Can't create a window, glfw error." << std::endl;
        const char* errorMessage;
        int code = glfwGetError(&errorMessage);
        std::cout << "GLFW error " << code << ": " << errorMessage << std::endl;
        return false;
    }

    glfwMakeContextCurrent(*window);

    // Initialize GLEW
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        std::cout << "Can't initialize GLEW." << std::endl;
        glfwTerminate();
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

    return true;
}

int main() {


    GLFWwindow* window;

    if(!initOpenGL(&window)) {

        glfwTerminate();
        return -1;
    }

    /*---------*/
    //LOADING SOUNDS
    /*---------*/

    FMOD::Studio::System *soundSystem{nullptr}; 
    FMOD::Studio::Bank *masterBank{nullptr}, *masterStringBank{nullptr};
    FMOD::Studio::EventDescription *descriptions{nullptr};

    loadSound(&soundSystem, &masterBank, &masterStringBank, &descriptions);

    FMOD_RESULT fmodResult;

    FMOD::Studio::EventInstance *explosioneventInstance{getSoundInstance("event:/grenade", descriptions, masterBank)};
    FMOD::Studio::EventInstance *stepeventInstance{getSoundInstance("event:/pas", descriptions, masterBank)};

    fmodResult = stepeventInstance->setParameterByName("Matiere", 0);
    fmodError(fmodResult, "event parameter \"Matiere\"");

    // fmodResult = explosioneventInstance->start();

    /*if (fmodResult != FMOD_OK) {

        std::cout << "Error " << fmodResult << " with FMOD studio API event start: " << FMOD_ErrorString(fmodResult) << std::endl;
        return -1;
    }*/
   

    /*---------*/
    /*---------*/



    // LOAD ENGINE
    Gg::GulgEngine engine;

    if(!engine.loadSignatures("Datas/Signatures")) {

        std::cout << "Gulg error: Can't open signatures file." << std::endl;
        return -1;
    }


    bool haveToStop{false};
    /* ----- */
    // LOAD SHADERS

    if(!engine.loadProgram("Datas/Shaders/voxelVertex.vert", "Datas/Shaders/voxelFragment.frag", "MainProgram")) {

        std::cout << "Error with shaders load." << std::endl;
        return -1;
    }

    if(!engine.loadProgram("Datas/Shaders/animationVertex.vert", "Datas/Shaders/animationFragment.frag", "AnimationProgram")) {

        std::cout << "Error with shaders load." << std::endl;
        return -1;
    }

    if(!engine.loadTexture("Datas/Animated/rambo.png", "RamboTexture")) {

        std::cout << "Error with shaders load." << std::endl;
        return -1;
    }

    GLuint program{engine.getProgram("MainProgram")};

    //INITIALISING PLAYER WORLD AND CAMERA //
    Gg::Entity gameID{engine.getNewEntity()},
               worldID{engine.getNewEntity()},
               cameraID{engine.getNewEntity()},
               playerID{engine.getNewEntity()};

    newMap(engine,worldID,program);

    std::shared_ptr<Gg::Component::SceneObject> gameScene{std::make_shared<Gg::Component::SceneObject>()};
    std::shared_ptr<Gg::Component::SceneObject> cameraScene{std::make_shared<Gg::Component::SceneObject>()};
    std::shared_ptr<Gg::Component::SceneObject> playerScene{std::make_shared<Gg::Component::SceneObject>()};

    std::shared_ptr<Gg::Component::Transformation> gameTransformation{std::make_shared<Gg::Component::Transformation>()};
    std::shared_ptr<Gg::Component::Transformation> cameraTransformation{std::make_shared<Gg::Component::Transformation>()};
    std::shared_ptr<Gg::Component::Transformation> playerTransformation{std::make_shared<Gg::Component::Transformation>()};

    std::shared_ptr<Gg::Component::Collider> playerCollider{std::make_shared<Gg::Component::Collider>
      (glm::vec3{0.f,0.f,0.f},glm::vec3{0.f,0.f,0.f},0.5f)};
    std::shared_ptr<Gg::Component::Forces> playerForces{std::make_shared<Gg::Component::Forces>()};


    engine.addComponentToEntity(gameID, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(gameScene));
    engine.addComponentToEntity(cameraID, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(cameraScene));
    engine.addComponentToEntity(playerID, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(playerScene));

    engine.addComponentToEntity(gameID, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(gameTransformation));
    engine.addComponentToEntity(cameraID, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(cameraTransformation));
    engine.addComponentToEntity(playerID, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(playerTransformation));

    engine.addComponentToEntity(playerID, "Collider", std::static_pointer_cast<Gg::Component::AbstractComponent>(playerCollider));
    engine.addComponentToEntity(playerID, "Forces", std::static_pointer_cast<Gg::Component::AbstractComponent>(playerForces));

    // loadAnimation(engine, playerID, "Datas/Animated/rambo.dae");
    // playerTransformation->translate(glm::vec3{0.f, 0.f, -20.f});
    // playerTransformation->scale(2);


     std::shared_ptr<Gg::Component::Mesh> playerMesh{std::make_shared<Gg::Component::Mesh>(program)};
    Cube(playerMesh,0.5f,glm::vec3{0.f,0.3f,1.0f});
         engine.addComponentToEntity(playerID, "MainMesh", std::static_pointer_cast<Gg::Component::AbstractComponent>(playerMesh));


    gameScene->addChild(worldID);
    gameScene->addChild(playerID);
    playerScene->addChild(cameraID);
    //INITIALISING LIGHTS
    Gg::Entity light1ID{engine.getNewEntity()};

    std::shared_ptr<Gg::Component::SceneObject> light1Scene{std::make_shared<Gg::Component::SceneObject>()};
    std::shared_ptr<Gg::Component::Transformation> light1Transformation{std::make_shared<Gg::Component::Transformation>()};
    std::shared_ptr<Gg::Component::Light> light1Light{std::make_shared<Gg::Component::Light>()};

    engine.addComponentToEntity(light1ID, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(light1Scene));
    engine.addComponentToEntity(light1ID, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(light1Transformation));
    engine.addComponentToEntity(light1ID, "Light", std::static_pointer_cast<Gg::Component::AbstractComponent>(light1Light));

    light1Light->m_ambient = glm::vec3{0.75f, 0.75f, 0.75f};
    light1Light->m_diffuse = glm::vec3{1.f, 1.f, 1.f};
    light1Light->m_specular = glm::vec3{1.f, 1.f, 1.f};

    light1Light->m_direction = glm::vec3{0.f, 0.f, -1.f};
    light1Light->m_lightType = Gg::Component::LightType::Directional;

    gameScene->addChild(light1ID);
    //gameScene->addChild(light2ID);

    //INITIALISING SYSTEMS
    UpdateScene sceneUpdate{engine};
    sceneUpdate.addEntity(gameID);

    DrawScene sceneDraw{engine};
    sceneDraw.addEntity(worldID);
    sceneDraw.addEntity(playerID);
    sceneDraw.setCameraEntity(cameraID);

    Physics physics{engine};
    physics.addEntity(playerID);

    Collisions collisions{engine,worldID,explosioneventInstance};
    collisions.addEntity(playerID);

    Time time{engine,worldID,explosioneventInstance};

    Lightning lightning{engine, program};
    lightning.addEntity(light1ID);
    //lightning.addEntity(light2ID);

    glm::mat4 projection{glm::perspective(glm::radians(45.0f), 800.f / 600.f, 0.1f, 2000.f)};
    //cameraTransformation->setSpecificTransformation(glm::lookAt(glm::vec3{0.f, 0.f, 10.f}, glm::vec3{0.f, 0.f, 0.f}, glm::vec3{0.f, 1.f, 0.f}));
    cameraTransformation->translate(glm::vec3{0.f, 0.f, -40.f});
     playerTransformation->translate(glm::vec3{0.f, 0.f, -20.f} * cameraTransformation->m_rotation);

    sceneDraw.setProjection(projection);


    // PARAMETERS (KEYS ,ACCELERATION,CURSOR MODE)
    float P_acc = 0.1f;

    int gOldState = GLFW_RELEASE;
    int gNewState = GLFW_RELEASE;
    int rOldState = GLFW_RELEASE;
    int rNewState = GLFW_RELEASE;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double oxpos, oypos,xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    double sensi=0.1f;
     // explosioneventInstance->setVolume(1.f);
    while (!haveToStop) {
        //Event
        oxpos = xpos;
        oypos = ypos;
        glfwGetCursorPos(window, &xpos, &ypos);


        glfwPollEvents();
        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { haveToStop = true; }

        if(oxpos != xpos || oypos != ypos){
          cameraTransformation->rotate(glm::radians(ypos-oypos)*sensi,  glm::vec3{1.f,0.f,0.f});
          cameraTransformation->rotate(glm::radians(xpos-oxpos)*sensi, glm::vec3{0.f,0.f,1.f}*glm::conjugate(cameraTransformation->m_rotation));
        }

        if(glfwGetKey(window, GLFW_KEY_LEFT ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(-1.f), glm::vec3{0.f, 1.f, 0.f});   }
        if(glfwGetKey(window, GLFW_KEY_RIGHT ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(1.f), glm::vec3{0.f, 1.f, 0.f});  }
        if(glfwGetKey(window, GLFW_KEY_UP ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(-1.f), glm::vec3{1.f, 0.f, 0.f});  }
        if(glfwGetKey(window, GLFW_KEY_DOWN ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(1.f), glm::vec3{1.f, 0.f, 0.f});  }
        if(glfwGetKey(window, GLFW_KEY_Q ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(-1.f), glm::vec3{0.f, 0.f, 1.f});  }
        if(glfwGetKey(window, GLFW_KEY_E ) == GLFW_PRESS) { cameraTransformation->rotate(glm::radians(1.f), glm::vec3{0.f, 0.f, 1.f});}

        if(glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) { playerForces->addForce(glm::vec3{0.f,  0.f,-(2.f*P_acc)} ); }
        if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) { playerForces->addForce(glm::vec3{0.f, 0.f, P_acc} ); }

        if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {  glm::vec3 toadd{glm::vec3{0.f, 0.f, P_acc} * cameraTransformation->m_rotation};        toadd[2]=0.f;    playerForces->addForce(toadd);   }
        if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {glm::vec3 toadd{glm::vec3{0.f, 0.f, -P_acc} * cameraTransformation->m_rotation};        toadd[2]=0.f;    playerForces->addForce(toadd); }

        if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { glm::vec3 toadd{glm::vec3{P_acc,0.f, 0.f} * cameraTransformation->m_rotation};        toadd[2]=0.f;    playerForces->addForce(toadd);  }
        if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {glm::vec3 toadd{glm::vec3{-P_acc,0.f, 0.f } * cameraTransformation->m_rotation};        toadd[2]=0.f;    playerForces->addForce(toadd);  }

        // if(glfwGetKey(window, GLFW_KEY_KP_1) == GLFW_PRESS) { eventInstance->setParameterByName("JojoSelection", 3); }
        // if(glfwGetKey(window, GLFW_KEY_KP_2) == GLFW_PRESS) { eventInstance->setParameterByName("JojoSelection", 4); }
        // if(glfwGetKey(window, GLFW_KEY_KP_3) == GLFW_PRESS) { eventInstance->setParameterByName("JojoSelection", 5); }
        //
        // if(glfwGetKey(window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
        //
        //     float currentVolume{0.f};
        //     eventInstance->getVolume(&currentVolume);
        //     eventInstance->setVolume(currentVolume*1.25f);
        // }
        //
        // if(glfwGetKey(window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
        //
        //     float currentVolume{0.f};
        //     eventInstance->getVolume(&currentVolume);
        //     eventInstance->setVolume(currentVolume*0.8f);
        // }


        gOldState = gNewState;
        gNewState = glfwGetKey(window, GLFW_KEY_G) ;
        //GRENADE
        if(gOldState == GLFW_PRESS && gNewState == GLFW_RELEASE ) {
          Gg::Entity newG{engine.getNewEntity()};
          std::shared_ptr<Gg::Component::SceneObject> newGScene{std::make_shared<Gg::Component::SceneObject>()};
          std::shared_ptr<Gg::Component::Transformation> newGTransformation{std::make_shared<Gg::Component::Transformation>()};
          std::shared_ptr<Gg::Component::Collider> newGCollider{std::make_shared<Gg::Component::Collider>
            (glm::vec3{0.f,0.f,0.f},glm::vec3{0.f,0.f,0.f},0.5f)};
          std::shared_ptr<Gg::Component::Forces> newGForces{std::make_shared<Gg::Component::Forces>()};
          std::shared_ptr<Gg::Component::Mesh> newGMesh{std::make_shared<Gg::Component::Mesh>(program)};
          Cube(newGMesh,0.5f,glm::vec3{1.f,0.f,0.f});
          std::shared_ptr<Gg::Component::Explosive> newGExp{std::make_shared<Gg::Component::Explosive>(5,TIMER)};
          std::shared_ptr<Gg::Component::Timer> newGTimer{std::make_shared<Gg::Component::Timer>(5000)};

          newGTransformation->setSpecificTransformation(playerScene->m_globalTransformations);
          glm::vec3 f {(glm::vec3{0.f, 0.f, 10.f} * cameraTransformation->m_rotation)};
          f[2] += -5.f;
          newGForces->addForce(playerForces->velocity + f);
          engine.addComponentToEntity(newG, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGScene));
          engine.addComponentToEntity(newG, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGTransformation));
          engine.addComponentToEntity(newG, "Collider", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGCollider));
          engine.addComponentToEntity(newG, "Forces", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGForces));
          engine.addComponentToEntity(newG, "MainMesh", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGMesh));
          engine.addComponentToEntity(newG, "Explosive", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGExp));
          engine.addComponentToEntity(newG, "Timer", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGTimer));
          gameScene->addChild(newG);
          sceneDraw.addEntity(newG);
          physics.addEntity(newG);
          collisions.addEntity(newG);
          time.addEntity(newG);
          sceneUpdate.applyAlgorithms();

        }


        rOldState = rNewState;
        rNewState = glfwGetKey(window, GLFW_KEY_R) ;
        //ROCKET
        if(rOldState == GLFW_PRESS && rNewState == GLFW_RELEASE ) {
          Gg::Entity newG{engine.getNewEntity()};
          std::shared_ptr<Gg::Component::SceneObject> newGScene{std::make_shared<Gg::Component::SceneObject>()};
          std::shared_ptr<Gg::Component::Transformation> newGTransformation{std::make_shared<Gg::Component::Transformation>()};
          std::shared_ptr<Gg::Component::Collider> newGCollider{std::make_shared<Gg::Component::Collider>
            (glm::vec3{0.f,0.f,0.f},glm::vec3{0.f,0.f,0.f},0.5f)};
          std::shared_ptr<Gg::Component::Forces> newGForces{std::make_shared<Gg::Component::Forces>(glm::vec3{0.f},0.f,1.f)};
          std::shared_ptr<Gg::Component::Mesh> newGMesh{std::make_shared<Gg::Component::Mesh>(program)};
          Cube(newGMesh,0.5f,glm::vec3{1.f,0.f,0.f});
          std::shared_ptr<Gg::Component::Explosive> newGExp{std::make_shared<Gg::Component::Explosive>(5,ON_COLLISION)};

          newGTransformation->setSpecificTransformation(playerScene->m_globalTransformations);
          glm::vec3 f {(glm::vec3{0.f, 0.f, 10.f} * cameraTransformation->m_rotation)};
          f[2] += -5.f;
          newGForces->addForce(playerForces->velocity + f);
          engine.addComponentToEntity(newG, "SceneObject", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGScene));
          engine.addComponentToEntity(newG, "Transformations", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGTransformation));
          engine.addComponentToEntity(newG, "Collider", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGCollider));
          engine.addComponentToEntity(newG, "Forces", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGForces));
          engine.addComponentToEntity(newG, "MainMesh", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGMesh));
          engine.addComponentToEntity(newG, "Explosive", std::static_pointer_cast<Gg::Component::AbstractComponent>(newGExp));
          gameScene->addChild(newG);
          sceneDraw.addEntity(newG);
          physics.addEntity(newG);
          collisions.addEntity(newG);
          sceneUpdate.applyAlgorithms();
        }





        //Update
        collisions.applyAlgorithms();
        time.applyAlgorithms();
        physics.applyAlgorithms();
        sceneUpdate.applyAlgorithms();

        //3D LISTENER ATTRIBUTES FOR SPATIALIZED SOUNDS
        FMOD_3D_ATTRIBUTES att3D_;
        att3D_.position = FMOD_VECTOR{playerScene->m_globalTransformations[3][0],playerScene->m_globalTransformations[3][1],playerScene->m_globalTransformations[3][2]};//position
        att3D_.velocity = FMOD_VECTOR{ 0.f,  0.f, 0.f };
        glm::vec3 f {cameraTransformation->m_rotation*glm::vec3{1.f,0.f,0.f}};   f=glm::normalize(f);  f*=-1;
        att3D_.forward = FMOD_VECTOR{ f[0],f[1],f[2]};
        glm::vec3 u {cameraTransformation->m_rotation*glm::vec3{0.f,0.f,1.f}};  u=glm::normalize(u);
        att3D_.up= FMOD_VECTOR{  u[0],u[1],u[2] };

        soundSystem->setListenerAttributes(0,&att3D_);


        lightning.applyAlgorithms();

        soundSystem->update();


        //Draw

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.15f, 0.75f, 0.95f, 1.0f);

        sceneDraw.applyAlgorithms();

        glfwSwapBuffers(window);




        //Entities to add/delete at the end of the current frame

        for(Gg::Entity toD : time.toDelete){
          collisions.deleteEntity(toD);
          physics.deleteEntity(toD);
          sceneUpdate.deleteEntity(toD);
          sceneDraw.deleteEntity(toD);
          engine.deleteEntity(toD);
          time.deleteEntity(toD);
          gameScene->deleteChild(toD);
        }
        for(Gg::Entity toD : time.toAdd){
          collisions.addEntity(toD);
          physics.addEntity(toD);
          sceneUpdate.addEntity(toD);
          sceneDraw.addEntity(toD);
          gameScene->addChild(toD);
        }

        for(Gg::Entity toD : collisions.toDelete){
          collisions.deleteEntity(toD);
          physics.deleteEntity(toD);
          sceneUpdate.deleteEntity(toD);
          sceneDraw.deleteEntity(toD);
          engine.deleteEntity(toD);
          time.deleteEntity(toD);
          gameScene->deleteChild(toD);
        }
        for(Gg::Entity toD : collisions.toAdd){
          collisions.addEntity(toD);
          physics.addEntity(toD);
          sceneUpdate.addEntity(toD);
          sceneDraw.addEntity(toD);
          gameScene->addChild(toD);
        }
        if(collisions.toAdd.size()>0 || time.toAdd.size()>0)sceneUpdate.applyAlgorithms();
        time.toAdd.clear();
        collisions.toAdd.clear();
        time.toDelete.clear();
        collisions.toDelete.clear();


    }

    glfwTerminate();

    return 0;
}
