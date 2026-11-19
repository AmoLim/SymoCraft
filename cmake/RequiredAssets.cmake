set(SYMOCRAFT_REQUIRED_ASSETS
    configs/blockFormats.yaml
    configs/npc_blocks.yaml
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
