set(SYMOCRAFT_REQUIRED_ASSETS
    configs/blockFormats.yaml
    configs/npc_blocks.yaml
    configs/npc_scene.yaml
    ai/human_harvest.xml
    ai/zombie_idle.xml
    ai/npc_nodes.xml
    characters/human.glb
    characters/zombie.glb
    textures/texture_atlas.png
)
if(NOT DEFINED SYMOCRAFT_RENDER_OPENGL OR SYMOCRAFT_RENDER_OPENGL)
    list(APPEND SYMOCRAFT_REQUIRED_ASSETS
    shaders/vs_BlockShader.glsl
    shaders/fs_BlockShader.glsl
    shaders/vs_FrameShader.glsl
    shaders/fs_FrameShader.glsl
    )
endif()
