# Neon Run: course checklist and code explanation

This is the small first-person **Torus Ride** suggested by the seminar. Use W/A/S/D to move up/left/down/right and keep the crosshair inside the next ring opening. The rings approach automatically. Passing an opening earns one point; missing increases the miss count. The path repeats until you close the game.

This guide explains steering, visible spin, a pass/miss counter and restart. A small white crosshair marks the centre of the screen. It consists of two line segments drawn relative to the camera. Scoring still uses the camera point. It uses the existing course framework and libraries. There is no additional engine or dependency.

Start with the [complete from-scratch lesson](/Users/sugatasarker/Developer/CG_Labs/ASSIGNMENT5_FROM_SCRATCH.md) for the window, triangles, maths, logic, shaders, crosshair and practice questions. This shorter guide keeps the rubric and line-by-line controls/scoring reference.

## Approval checklist

Source: [Seminar 5, slide 30 (PDF page 36)](/Users/sugatasarker/Downloads/sem_5.pdf), plus [the assignment brief, section 2](/Users/sugatasarker/Downloads/Assignment5_GameProject.pdf). These documents give minimum requirements, not a numerical points rubric or a guarantee of approval.

| Requirement | How our game addresses it | What to show |
| --- | --- | --- |
| Ship/camera manoeuvrability | WASD changes camera X/Y; camera Z remains 8. | Steer towards differently positioned rings. |
| Tessellated objects with shaders | `createTorus` makes a triangle mesh; our vertex/fragment shaders provide diffuse lighting and colour. | Select **Polygon mode → Line**, then return to **Fill**. |
| Translational animation | Each ring moves along +Z by `ring_speed * dt`. | Watch rings approach. |
| Rotational animation | Each ring calls `RotateZ(spin_speed * dt)`. An orange segment makes the spin visible. | Watch the orange segment move around a ring. |
| Fixed object array | `std::array<Node, 8>` stores eight rings, sharing one mesh. Rings recycle 48 units backwards. | Explain the array and recycling loop. |
| Game presentation at the lab and in the forum gallery | Still to be completed by you. | Demonstrate the game and prepare the gallery post. |
| Stable enough to run and test | Build, gameplay checks and a live play-through are the relevant checks. | Follow the short demo checklist below. |

The brief also asks for a simple game mechanic. Our pass/miss feedback provides that. The seminar marks game states, collision detection, physics simulation and score count as **optional**. We added just a simple score and restart. A timer, physics engine, imported ship, textures, random path and game-over screen are not required by this minimum list.

## Which ideas come from the course?

| Code idea | Course connection |
| --- | --- |
| Torus Ride, a fixed array and recycling | Seminar 5, slides 5 and 8. |
| Planar WASD control | Seminar 5, slide 6 permits movement constrained to a plane; slide 24 explains key input. Held-key logic is already in `src/core/FPSCamera.inl`. |
| Translation and rotation using elapsed seconds | Assignment 1 / `CelestialBody.cpp`; seminar animation guidance. |
| Tessellated torus, positions, normals and indices | Assignment 2's parametric-mesh approach, already used for our torus. |
| World transforms, normals and diffuse lighting | Assignment 3's `diffuse.vert` / diffuse shader. |
| Passing an extra value from vertex to fragment shader | Same vertex-output/fragment-input mechanism as Assignment 3; our extra value is the original vertex position. |
| Two line segments for the crosshair | Vertex buffers and `GL_LINES`; the same position attribute and transform patterns as the labs. |
| Vectors, length and normalisation | Course graphics mathematics; the seminar's sphere tests also compare distances. |
| ImGui feedback and controls | Existing lab interface; seminar 5, slide 26 recommends ImGui for feedback. |

The following are **small choices we made for this game**, not code copied from a slide:

- The eight X/Y offsets form an easy, repeating path.
- The orange segment uses `local_vertex.y > 1.4`. This is an ordinary shader `if` using model-space coordinates.
- The pass test compares distance in the XY plane with the opening radius. It is an aperture-crossing rule for a point camera, **not** the seminar's sphere-sphere test or full torus collision detection.
- Linear interpolation estimates where the player was at the exact crossing instant: `old + t * (new - old)`. This is the same basic interpolation idea as the earlier assignments.
- Normalising movement keeps diagonals at the same speed. `clamp` limits camera X/Y to [-4, 4]. `min` limits a delayed frame to 0.1 seconds. These are simple helpers: clamp keeps a value inside an interval; min chooses the smaller of two numbers. After a long stall the game deliberately advances less than wall-clock time.
- Restart uses a Boolean request and resets existing variables. There is no new state-machine architecture.

## Explain the game in one minute

> I used the Torus Ride idea from the seminar. I made a tessellated torus and reused it in eight Nodes. Every Node has its own position and rotation. The camera moves in the XY plane using WASD. The rings move towards the camera using speed multiplied by frame time, and recycle behind the other rings. They rotate about Z; an orange segment in the shader makes this visible. When a ring crosses the camera's Z position, I check whether the camera is inside the opening and update the score. The controls and feedback use the existing ImGui interface.

## Read these parts in order

1. [src/EDAF80/assignment5.cpp:107](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:107) — sizes and variables.
2. [src/EDAF80/assignment5.cpp:218](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:218) — initial placement and restart.
3. [src/EDAF80/assignment5.cpp:232](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:232) — steering, movement, spin and scoring.
4. [shaders/EDAF80/neon_ring.vert:11](/Users/sugatasarker/Developer/CG_Labs/shaders/EDAF80/neon_ring.vert:11) and [shaders/EDAF80/neon_ring.frag:5](/Users/sugatasarker/Developer/CG_Labs/shaders/EDAF80/neon_ring.frag:5) — why the orange part rotates with the ring.
5. [src/EDAF80/assignment5.cpp:301](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:301) — feedback and controls.

## C++ notation used here

- `float` stores a decimal number; `4.0f` is a float literal. `int` stores whole-number counters. `bool` stores true/false.
- `const` means we will not change this value after initialisation.
- `auto` asks C++ to infer a type from the right side. `auto& ring` refers to the actual array element instead of copying it.
- `glm::vec2` stores X/Y; `glm::vec3` stores X/Y/Z. Constructing `vec2` from `vec3` keeps only X/Y.
- `+=` adds to a value; `++score` adds one. `!` means not and `&&` means both conditions must be true.
- In `key_state & PRESSED`, `&` is a bitwise check for the pressed flag. This differs from `&ring_speed`, which gives ImGui the variable's address so it can edit it.
- `std::size_t` is the array index type; `0u` is unsigned zero. `static_cast<float>(i)` explicitly converts an index to a float for the position calculation.
- `char const* last_result` points to one of our fixed text messages. We change which message it points to, not the text itself; no manual allocation is involved.
- `{ ... }` initialises `std::array` and its contained elements. Ordinary braces delimit functions, loops and `if` blocks; a closing brace ends the innermost block.
- The existing shader callback `[](GLuint program) { ... }` is a small function passed to `set_program`. `Node::render` calls it while the shader is active, allowing us to upload the light position.

## The opening test, with numbers

A torus has major radius `R = 2.0` and tube radius `r = 0.25`. Its opening at the central plane therefore has radius `R - r = 1.75`.

At the crossing instant, compute the XY difference between camera and ring centre, then `glm::length` calculates `sqrt(dx*dx + dy*dy)`. A distance below 1.75 earns one point. A distance equal to or above 1.75 counts as a miss. The camera is treated as a point.

The previous/current Z comparison only becomes true once on each approach. We score before recycling, so recycling cannot itself award a point. Because the rings only spin around Z, their openings stay circular in the XY plane. Tilting around X/Y or modelling a ship with a radius would require changing this simple rule.

For interpolation, if a ring goes from Z=7.9 to Z=8.1, it crosses camera Z=8 halfway through the frame: `(8 - 7.9)/(8.1 - 7.9) = 0.5`. We use the player's halfway position. This avoids scoring from a position reached only after the ring had already passed.

## Line-by-line explanation of this checkpoint

Line numbers below refer to the source saved with this guide. Blank lines separate ideas; comments describe the nearby code. Unchanged framework startup, event polling, render setup and shutdown remain as in the lab skeleton.

The window title is now `Neon Run`. The shader registration now loads `EDAF80/neon_ring.vert`; our original shared `diffuse.vert` is unchanged. Ring placement moved from the geometry-setup loop into the restart block, so both first launch and restart use the same positioning code.

### Sizes and settings

| Line | Code | Explanation |
| --- | --- | --- |
| 107 | `float const ring_radius = 2.0f;` | Distance from the ring centre to the centreline of its tube: 2 units. |
| 108 | `float const tube_radius = 0.25f;` | Radius of the tube itself: 0.25 units. |
| 109 | `float const opening_radius = ring_radius - tube_radius;` | Subtracts tube thickness from the major radius to get the 1.75-unit opening. |
| 110 | `auto const ring_mesh = parametric_shapes::createTorus(ring_radius, tube_radius, 63u, 15u);` | Creates one triangle mesh using these radii and the existing tessellation counts. |
| 111 | `if (ring_mesh.vao == 0u) {` | Detects a failed mesh creation before trying to draw it. |
| 112 | `LogError("Failed to create torus geometry");` | Reports the mesh-creation failure in the course log. |
| 113 | `return;` | Leaves the run function if mesh creation failed. |
| 114 | `}` | Ends the current loop or conditional block. |
| 131 | `std::array<Node, 8> rings;` | Creates exactly eight scene objects; this array does not grow during play. |
| 133 | `std::array<glm::vec2, 8> const ring_offsets = {{` | Stores eight fixed X/Y centre positions in matching ring order. |
| 134 | `glm::vec2( 0.0f,  0.0f), glm::vec2( 2.0f,  0.0f),` | Ring 0 starts at the centre; ring 1 is 2 units to the right. |
| 135 | `glm::vec2( 2.0f,  2.0f), glm::vec2( 0.0f,  2.0f),` | Ring 2 is right and up; ring 3 is directly up. |
| 136 | `glm::vec2(-2.0f,  2.0f), glm::vec2(-2.0f,  0.0f),` | Ring 4 is left and up; ring 5 is directly left. |
| 137 | `glm::vec2(-2.0f, -2.0f), glm::vec2( 0.0f, -2.0f)` | Ring 6 is left and down; ring 7 is directly down. |
| 138 | `}};` | Closes the fixed-array initialiser and ends the declaration. |
| 139 | `float const ring_spacing = 6.0f;` | Puts neighbouring rings 6 units apart along Z. |
| 140 | `float const ring_start_z = -12.0f;` | Starts the first ring at Z=-12, in front of camera Z=8. |
| 141 | `float const ring_end_z = 10.0f;` | Recycles a ring after its centre reaches Z=10, behind the camera. |
| 142 | `float const loop_length = ring_spacing * static_cast<float>(rings.size());` | Computes 6 times 8 = 48 units for each recycling move. |
| 143 | `float ring_speed = 4.0f; // World units per second.` | Starts forward ring speed at 4 units per second; the UI can change it. |
| 144 | `float const steering_speed = 4.0f;` | Sets camera movement speed to 4 units per second. |
| 145 | `float const spin_speed = 0.8f; // Radians per second.` | Sets ring angular speed to 0.8 radians per second. |
| 146 | `bool pause_motion = false;` | Starts with movement enabled. |
| 147 | `bool restart_requested = true; // Also set up the first run.` | Requests initial placement on the first frame; the same code handles later restarts. |
| 148 | `int score = 0;` | Starts the successful-crossing counter at zero. |
| 149 | `int missed_rings = 0;` | Starts the missed-crossing counter at zero. |
| 150 | `char const* last_result = "Steer through the ring openings.";` | Selects the initial instruction message for the UI. |

### Frame time

| Line | Code | Explanation |
| --- | --- | --- |
| 182 | `float const dt = glm::min(std::chrono::duration<float>(deltaTimeUs).count(), 0.1f);` | Converts elapsed time to seconds, then takes the smaller of that time and 0.1 seconds. |

### Restart and gameplay update

| Line | Code | Explanation |
| --- | --- | --- |
| 218 | `if (restart_requested) {` | Runs setup only on the first frame or after the Restart button. |
| 219 | `mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 8.0f));` | Moves the camera back to the centre at Z=8. |
| 220 | `score = 0;` | Clears successful crossings for the new run. |
| 221 | `missed_rings = 0;` | Clears misses for the new run. |
| 222 | `last_result = "Steer through the ring openings.";` | Restores the initial instruction message. |
| 223 | `pause_motion = false;` | Resumes movement when starting or restarting. |
| 224 | `for (std::size_t i = 0u; i < rings.size(); ++i) {` | Visits ring indices 0 through 7, adding one to i after each iteration. |
| 225 | `float const z = ring_start_z - ring_spacing * static_cast<float>(i);` | Computes -12, -18, -24, ..., -54 from the ring index. |
| 226 | `rings[i].get_transform().SetTranslate(glm::vec3(ring_offsets[i], z));` | Combines the stored X/Y offset and calculated Z, then positions this ring. |
| 227 | `rings[i].get_transform().SetRotateZ(0.0f);` | Resets this ring to zero rotation. |
| 228 | `}` | Ends the current loop or conditional block. |
| 229 | `restart_requested = false;` | Consumes the request so setup is not repeated every frame. |
| 230 | `}` | Ends the current loop or conditional block. |
| 232 | `if (!pause_motion && !shader_reload_failed) {` | Updates gameplay only while unpaused and the shader is working. |
| 233 | `auto const previous_camera_position = mCamera.mWorld.GetTranslation();` | Saves the camera position from the beginning of this frame. |
| 234 | `auto camera_position = previous_camera_position;` | Makes a changeable copy to accumulate this frame's movement. |
| 235 | `glm::vec3 direction(0.0f);` | Starts movement direction at (0,0,0), meaning no movement. |
| 237 | `if (!inputHandler.IsKeyboardCapturedByUI()) {` | Lets UI keyboard interaction take priority over steering. |
| 238 | `if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) direction.x -= 1.0f;` | While A is held, add a leftwards X component. |
| 239 | `if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) direction.x += 1.0f;` | While D is held, add a rightwards X component. |
| 240 | `if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) direction.y += 1.0f;` | While W is held, add an upwards Y component. |
| 241 | `if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) direction.y -= 1.0f;` | While S is held, add a downwards Y component. Opposite keys cancel. |
| 242 | `}` | Ends the current loop or conditional block. |
| 244 | `if (glm::length(direction) > 0.0f) {` | Checks that a direction exists before normalising; zero has no direction. |
| 245 | `camera_position += glm::normalize(direction) * steering_speed * dt;` | Adds unit direction times speed times seconds, keeping diagonal speed equal. |
| 246 | `}` | Ends the current loop or conditional block. |
| 247 | `camera_position.x = glm::clamp(camera_position.x, -4.0f, 4.0f);` | Restricts the camera X coordinate to between -4 and 4. |
| 248 | `camera_position.y = glm::clamp(camera_position.y, -4.0f, 4.0f);` | Restricts the camera Y coordinate to between -4 and 4. Z is never changed. |
| 249 | `mCamera.mWorld.SetTranslate(camera_position);` | Applies the calculated position without changing camera orientation. |
| 251 | `for (auto& ring : rings) {` | Visits each actual ring by reference so transform changes are retained. |
| 252 | `auto position = ring.get_transform().GetTranslation();` | Copies the current ring centre so we can calculate its next position. |
| 253 | `float const previous_z = position.z;` | Remembers the ring's Z before moving it, for the crossing test. |
| 254 | `position.z += ring_speed * dt;` | Moves the ring along +Z by speed multiplied by elapsed seconds. |
| 255 | `ring.get_transform().RotateZ(spin_speed * dt);` | Adds angular speed times elapsed seconds to the ring's Z rotation. |
| 258 | `if (previous_z < camera_position.z && position.z >= camera_position.z) {` | Detects the single frame in which the ring moves from in front of the camera to its plane or behind it. |
| 259 | `float const crossing_fraction = (camera_position.z - previous_z) / (position.z - previous_z);` | Finds what fraction of this movement occurred before the crossing; the crossing condition guarantees positive Z movement, so this denominator is nonzero. |
| 260 | `auto const crossing_position = previous_camera_position` | Starts the interpolated position calculation at the old camera position. |
| 261 | `+ crossing_fraction * (camera_position - previous_camera_position);` | Adds the crossing fraction of the camera's full movement this frame. |
| 262 | `float const distance = glm::length(glm::vec2(crossing_position - position));` | Drops Z and measures the XY distance from the ring centre to the interpolated camera position. |
| 263 | `if (distance < opening_radius) {` | Checks whether that point fits strictly inside the opening. |
| 264 | `++score;` | Adds one successful crossing. |
| 265 | `last_result = "Through the opening! +1 point";` | Selects the success message. |
| 266 | `} else {` | Handles the other case: the camera is on or outside the opening edge. |
| 267 | `++missed_rings;` | Adds one missed crossing. |
| 268 | `last_result = "Missed. Aim for the next opening.";` | Selects the miss message. |
| 269 | `}` | Ends the current loop or conditional block. |
| 270 | `}` | Ends the current loop or conditional block. |
| 273 | `while (position.z >= ring_end_z) {` | Checks whether the ring has reached the recycling boundary. |
| 274 | `position.z -= loop_length;` | Moves it 48 units back, preserving its X/Y offset. |
| 275 | `}` | Ends the current loop or conditional block. |
| 276 | `ring.get_transform().SetTranslate(position);` | Saves the final moved or recycled ring position. |
| 277 | `}` | Ends the current loop or conditional block. |
| 278 | `}` | Ends the current loop or conditional block. |

### Game interface

| Line | Code | Explanation |
| --- | --- | --- |
| 301 | `bool const opened = ImGui::Begin("Neon Run", nullptr, ImGuiWindowFlags_AlwaysAutoResize);` | Starts the existing ImGui control window with the final game title. |
| 302 | `if (opened) {` | Adds widgets only while the panel is expanded. |
| 303 | `ImGui::TextUnformatted("W/A/S/D: up / left / down / right");` | Displays the steering controls. |
| 304 | `ImGui::TextUnformatted("Keep the crosshair inside each opening.");` | Explains where to aim while playing. |
| 305 | `ImGui::Text("Score: %d \| Missed: %d", score, missed_rings);` | Displays the two integer counters, replacing the two %d placeholders. |
| 306 | `ImGui::TextUnformatted(last_result);` | Displays whichever instruction/success/miss message is currently selected. |
| 307 | `if (ImGui::Button("Restart")) restart_requested = true;` | Requests a restart on a button click. The reset runs at the start of the next update. |
| 308 | `ImGui::Checkbox("Pause", &pause_motion);` | Lets the checkbox change the pause Boolean. |
| 309 | `ImGui::SliderFloat("Speed (units/s)", &ring_speed, 1.0f, 10.0f);` | Lets the slider change ring speed within 1 to 10 units per second. |
| 310 | `bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);` | Keeps the course Fill / Line / Point dropdown you asked for. |
| 311 | `ImGui::Checkbox("Show basis", &show_basis);` | Keeps the existing optional coordinate-axis display. |
| 312 | `ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);` | Keeps the existing axis thickness control. |
| 313 | `ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);` | Keeps the existing axis length control. |
| 314 | `}` | Ends the current loop or conditional block. |
| 315 | `ImGui::End();` | Ends the ImGui window, including when it is collapsed. |

### neon_ring.vert

| Line | Code | Explanation |
| --- | --- | --- |
| 1 | `#version 410` | Uses GLSL 4.10, matching the course OpenGL context. |
| 4 | `layout (location = 0) in vec3 vertex;` | Receives the mesh vertex position from attribute 0. |
| 5 | `layout (location = 1) in vec3 normal;` | Receives the mesh normal from attribute 1. |
| 7 | `uniform mat4 vertex_model_to_world;` | Receives the model matrix supplied by Node, including position and rotation. |
| 8 | `uniform mat4 normal_model_to_world;` | Receives Node's inverse-transpose matrix for transforming normals. |
| 9 | `uniform mat4 vertex_world_to_clip;` | Receives the camera view-projection matrix. |
| 11 | `out VS_OUT {` | Begins the vertex shader output block. |
| 12 | `vec3 vertex;` | Declares the vertex position in world space for lighting. |
| 13 | `vec3 normal;` | Declares the normal in world space for lighting. |
| 14 | `vec3 local_vertex;` | Adds the untransformed model-space position used for the coloured segment. |
| 15 | `} vs_out;` | Names the vertex shader output instance vs_out. |
| 17 | `void main()` | Defines the shader entry function, run for each vertex or fragment. |
| 18 | `{` | Starts the function body. |
| 19 | `vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));` | Transforms the vertex into world space; w=1 includes translation. |
| 20 | `vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));` | Transforms the normal; w=0 represents a direction rather than a point. |
| 21 | `vs_out.local_vertex = vertex;` | Copies the original vertex position before rotation/translation, so the colour is tied to the object. |
| 22 | `gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);` | Transforms the vertex through model and view-projection matrices to clip space. |
| 23 | `}` | Ends the current conditional or function body. |

### neon_ring.frag

| Line | Code | Explanation |
| --- | --- | --- |
| 1 | `#version 410` | Uses GLSL 4.10, matching the course OpenGL context. |
| 3 | `uniform vec3 light_position;` | Receives the point-light position uploaded by the C++ shader callback. |
| 5 | `in VS_OUT {` | Begins the matching fragment shader input block; values are interpolated over each triangle. |
| 6 | `vec3 vertex;` | Declares the vertex position in world space for lighting. |
| 7 | `vec3 normal;` | Declares the normal in world space for lighting. |
| 8 | `vec3 local_vertex;` | Adds the untransformed model-space position used for the coloured segment. |
| 9 | `} fs_in;` | Names the fragment shader input instance fs_in. |
| 11 | `out vec4 frag_colour;` | Declares the fragment shader's final RGBA colour output. |
| 13 | `void main()` | Defines the shader entry function, run for each vertex or fragment. |
| 14 | `{` | Starts the function body. |
| 15 | `vec3 N = normalize(fs_in.normal);` | Normalises the interpolated surface normal to length one. |
| 16 | `vec3 L = normalize(light_position - fs_in.vertex);` | Finds and normalises the direction from the surface point to the light. |
| 17 | `float diffuse = max(dot(N, L), 0.0);` | Computes Lambert diffuse brightness with dot(N,L); max prevents negative light. |
| 18 | `vec3 ring_colour = vec3(0.05, 0.8, 1.0);` | Starts with a cyan RGB colour. |
| 20 | `if (fs_in.local_vertex.y > 1.4) {` | Selects the upper part of the ring in its original model coordinates. |
| 21 | `ring_colour = vec3(1.0, 0.45, 0.08);` | Makes that segment orange. It follows the mesh as the model matrix rotates it. |
| 22 | `}` | Ends the current conditional or function body. |
| 24 | `frag_colour = vec4(ring_colour * (0.18 + 0.82 * diffuse), 1.0);` | Scales the colour by ambient plus diffuse light and sets alpha to 1 (opaque). |
| 25 | `}` | Ends the current conditional or function body. |

## Run and demonstrate

Build from Terminal:

```sh
/opt/homebrew/bin/cmake --build /Users/sugatasarker/Developer/CG_Labs-build --config Debug --target EDAF80_Assignment5
```

Run:

```sh
/Users/sugatasarker/Developer/CG_Labs-build/src/EDAF80/Debug/EDAF80_Assignment5
```

The current local build loads shaders from the Developer project. Keep that shader folder available; this is not yet a self-contained distribution package.

1. Leave speed at 4. The first ring should reach the camera after about five seconds.
2. Hold A/D and W/S to demonstrate movement in both axes. Keep the crosshair inside the next opening.
3. Deliberately pass one ring and miss another; show the counters and feedback.
4. Show the orange segment rotating and the whole ring approaching.
5. Pause, change **Polygon mode** to **Line**, and explain the triangles. Return to **Fill** and resume.
6. Click Restart and show that score, misses, camera and ring placement reset.
7. Play for at least a minute to show recycling. F2 hides/shows the UI; F3 toggles logs; R reloads shaders; F11 toggles fullscreen.

## Gallery draft to finish yourself

**Title:** Neon Run

**Creators:** [add your name and any partner's name]

**Objective:** Steer through the approaching ring openings and collect as many points as possible.

**Features and implementation:** Eight tessellated tori share one mesh and are stored in a fixed array. Each ring translates towards the camera, rotates around Z and recycles after passing. WASD controls the camera in the XY plane. A crosshair made from two line segments is attached to the camera and marks the screen centre. The shaders use diffuse lighting and an orange segment to show rotation. A distance check at the ring's crossing plane updates the score or miss counter. ImGui provides feedback, pause, restart, speed and polygon-mode controls.

**Media available:** [Fill-mode render](/Users/sugatasarker/Developer/CG_Labs/docs/assignment5/gameplay.png) and [Line-mode render](/Users/sugatasarker/Developer/CG_Labs/docs/assignment5/wireframe.png), captured from a deterministic run of the real rendering code. You can also record a short video showing steering and rotation. Read the actual Canvas discussion description before posting. This draft has not been posted.
