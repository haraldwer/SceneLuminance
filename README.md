# SceneLuminance
An unreal-plugin that lets the user sample scene-cubemaps on the CPU. Made specifically for virtual video production in combination with DMX lights, but can be used for other things as well.

Only tested in Unreal 5.4. 
WARNING: Might only work in PIE. 

Might work extra well in combination with: 
https://github.com/zurra/LXR-Free

# Setup
Install the plugin like any other:
https://dev.epicgames.com/documentation/en-us/unreal-engine/plugins-in-unreal-engine

# Usage
1. Place a ```SceneLuminanceCaptureComponent``` at the location you wish to sample.
2. Set "Texture Target" to a ```CubeRenderTarget```, for example T_LuminanceRenderTarget
3. To sample the scene, use ```GetLuminance()``` in ```SceneLuminanceCaptureComponent```
4. Press play!

# Demo
Place the BP_LuminanceSampleTest-pawn in your scene. 

When entering play-mode, a debug-widget will be added to the screen:
Top left - The blurred luminance-values in all directions relative to the camera
Bottom left - The scene-capture and a red dot representing the uv-coordinate of the current camera direciton.

# How it works
The scene is rendered to a CubeRenderTarget. This TextureCube only exists on the GPU and has to be copied to the CPU in order for us to read it. A TextureCube has 6 textures, one for each side of the cube. Each texture is coped in a TArray<FLinearColor> using FRenderTarget::ReadLinearColorPixels(). 

The tricky part is mapping the sample direction (in 3D) to the correct cube face and face UV. My solution was inspired by this, but in reverse: https://stackoverflow.com/a/34427087

# Future work
Copying the pixels to CPU should not be done on the game thread, but I found no other way that was thread-safe.
Investigate the math behind luminance and ambient lighting further. 
