//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_APPLICATION_H
#define SYMOCRAFT_APPLICATION_H


struct GLFWwindow;
namespace SymoCraft
{
    struct StartupOptions;
    struct Window;
    //struct FrameBuffer;
    class GlobalThreadPool;
    class Camera;
    namespace ECS
    {
        class Registry;
    }

    namespace Application
    {
        // Initializing application
        void Init(const StartupOptions& options);

        // Run application
        void Run(const StartupOptions& options);
        void PrintWorldSummary(const StartupOptions& options);

        // Free application
        void Free();

        // Get Window Reference
        Window& GetWindow();
        //
        // Get Global Thread Pool Reference
        GlobalThreadPool& GetGlobalThreadPool();

        // Get Camera Pointer
        Camera* GetCamera();

        // Get Registry
        ECS::Registry &GetRegistry();

        // Process input and some functions call back
        void processInput(GLFWwindow* window);
        void MouseMovementCallBack(GLFWwindow* window, double x_pos_in, double y_pos_in);
        void MouseScrollCallBack(GLFWwindow* window, double x_pos_in, double y_pos_in);


        // Delta time
        extern float delta_time;
    }
}

#endif //SYMOCRAFT_APPLICATION_H
