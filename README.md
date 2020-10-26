*** 193541T ***

A1 features implemented:
- Skydome (replacing sky plane)
- Multi-texturing
- Terrain
- Fog
- Sprite Animation
- Instancing
- Water graphics (oops, supposed to be for A2)

A2 features implemented:
- Billboarding (regular method and shader method)
- Directional light shadows
- Spotlight shadows (works but not in the right way)
- Solved shadow acne with shadow bias
- Solved Peter Panning with front-face culling
- Soft shadows using Percentage-Closer Filtering (PCF)
- HDR rendering
- Bloom with Gaussian blur
- 3 particle systems (Rain, Smoke, Swirl)
- Object pooling
- Custom colour and opacity for particles
- Batch rendering (all particles are rendered with 1 draw call)

Other stuff done:
- Self-written framework and shaders
- No warnings (excluding files from external libraries), errors and memory leaks
- Simple terrain collision
- Terrain normals
- Rendering of terrain normals with a geometry shader
- Ray casting (for quads only)
- Object outlining with stencil buffer
- Dynamic environment mapping (works but not in the right way)
- Post-processing effects
- Camera movement and camera zoom with mouse
- Use of texture maps on models
- Blinn-Phong reflection model
- Gamma correction

Controls:
- ESC: End programme
- SPACE: Show or hide terrain normals
- ENTER: Change fog type
- WASD: Move camera forward, left, backward and right respectively
- Q and E: Move camera up and down respectively
- R: Reset camera
- 1: Toggle face culling
- 2: Cycle polygon modes
- 3: Change post-processing effects
- Left Mouse Button (LMB): Move camera towards target
- Right Mouse Button (RMB): Move camera away from target/Put out fire of campfire when camera is pointing at it
- Mouse scroll wheel: Zoom