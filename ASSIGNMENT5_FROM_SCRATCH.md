# Learn Neon Run from scratch

> Current controls: WASD accelerates, releasing coasts, and Space brakes. Hitting a ring ends the game. Read [the physics lesson](ASSIGNMENT5_PHYSICS.md) for current movement and collision code. The original lesson below describes the earlier version; its direct-speed examples and line numbers are historical.

Our game is a small **Torus Ride**: you steer up, down, left and right while rings approach. Go through their openings to earn points. There is no timer, lives system or game-over condition. A miss adds to the miss counter, then you continue. A small white crosshair marks the centre of the screen. Steer so this mark stays inside the approaching opening. Scoring still treats the camera as a point, so the aiming marker adds no collision physics or extra difficulty.

Read this lesson in order. After each worked example, stop and try the short check before reading its answer. You do not need to memorise every OpenGL function name; you should understand what data we create, how it changes, and how it becomes a picture. The separate [line-by-line guide and requirement checklist](/Users/sugatasarker/Developer/CG_Labs/ASSIGNMENT5_GUIDE.md) has the detailed controls/scoring tables.

## 1. What we wrote, and what the course already supplied

We built this **inside the existing course framework**. “From scratch” here means understanding how our game is assembled; it does not mean that we wrote the window system or OpenGL ourselves.

| Piece | What it does | Where it comes from |
| --- | --- | --- |
| `Bonobo`, `WindowManager` | Start the graphics framework, create the window, connect input and ImGui. | Course framework. |
| `InputHandler` and GLFW | Record keyboard state and window events. | Course framework / supplied library. |
| `FPSCameraf` | Stores the camera transform and creates view/projection matrices. | Course framework. We use our own simple planar steering instead of its free-flight update. |
| `Node` and `TRSTransform` | Associate a mesh with a shader and a position, rotation and scale; draw it. | Course framework. |
| `ShaderProgramManager` | Loads, compiles, links and reloads GLSL shaders. | Course framework. |
| GLM | Vector, matrix, trigonometric and basic maths operations. | Library already used by the course. |
| ImGui | Draws text, buttons, sliders and checkboxes. | Library already used by the course. |
| `createTorus` | Calculates ring vertices, normals and triangles, and uploads them. | Our implementation using the earlier parametric-mesh pattern. |
| Ring shaders | Transform vertices, calculate diffuse lighting and colour an orange segment. | Based on the course diffuse-shader pattern, with our colour choice. |
| Gameplay in `Assignment5::run()` | Ring path, steering, movement, spin, recycling and score. | Our game logic using course concepts. |
| `createCrosshair` and crosshair setup | Two line segments and a camera-relative transform. | Our small addition, using the course vertex-buffer and transform patterns. |
| Crosshair shaders | Transform vertices and output solid white. | Existing `common/fallback` shaders, reused unchanged. |

The course minimum checklist is in [Seminar 5, slide 30 / PDF page 36](/Users/sugatasarker/Downloads/sem_5.pdf). It asks for manoeuvrability, tessellated objects with shaders, translation and rotation, a fixed object array, and a presentation at the lab and in the gallery. Our [assignment brief](/Users/sugatasarker/Downloads/Assignment5_GameProject.pdf) also asks for a simple playable mechanic. These are minimum requirements, not a numerical grade guarantee. The lab demonstration and gallery post are still your tasks.

## 2. Understand the world before reading the loop

A `glm::vec3` holds three numbers: `(x, y, z)`.

- `x` changes left/right.
- `y` changes down/up.
- Our camera looks towards negative `z`.

The camera begins at `(0, 0, 8)`. The first ring begins at `(0, 0, -12)`, so it is **20 world units ahead** of the camera. The ring moves towards positive Z. The camera's Z stays 8; WASD changes only its X and Y.

This gives the feeling of forward flight without moving the camera forward forever. What matters visually is the relative position of camera and rings.

At the default ring speed of 4 units/second, the first ring takes:

```text
distance = 8 - (-12) = 20 units
time = distance / speed = 20 / 4 = 5 seconds
```

When you move the camera right, a ring at a fixed world X appears to move left on the screen. That is what should happen: you are changing your viewpoint.

**Check:** If camera X is 1 and a ring centre X is 3, is that ring left or right of your viewing direction? **Answer:** right, because its relative X is `3 - 1 = 2`.

## 3. The program's route from launch to exit

Start at [main, line 335](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:335).

```cpp
std::setlocale(LC_ALL, "");
Bonobo framework;
try {
    edaf80::Assignment5 assignment5(framework.GetWindowManager());
    assignment5.run();
} catch (std::runtime_error const& e) {
    LogError(e.what());
}
```

Line by line:

1. `setlocale` selects the system locale. It is startup housekeeping, not gameplay.
2. Constructing `framework` starts the course framework.
3. `try` begins code that can report a runtime failure through an exception.
4. Constructing `assignment5` passes it the framework's window manager.
5. `run()` sets up the scene and stays in the frame loop until the window closes.
6. `catch` receives a `std::runtime_error` if one is thrown.
7. `LogError(e.what())` displays its message.

The [constructor, line 53](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:53) initialises the camera with a vertical field of view of `0.5 × π/2 = π/4 = 45°`, the window's width/height ratio, and near/far clipping distances of `0.01` and `1000`. It creates the window, rejects a null window pointer, and calls `bonobo::init()`. The destructor calls `bonobo::deinit()` when the assignment object goes out of scope.

The includes at the top make declarations available: our assignment class and parametric shapes; framework camera, nodes, shaders and helpers; GLM's pointer helper; ImGui; tinyfiledialogs for reload errors; and standard C++ arrays, timing, locale, size and exception types. An include does not itself create a ring or run a game loop.

Inside `run`, the overall order is:

```text
ONCE: position camera → load shaders → create meshes → create Nodes → initialise settings
EACH FRAME: measure time → read input → restart/update gameplay → draw world → draw UI → display
AT EXIT: delete our GPU mesh resources
```

Keep **setup**, **update** and **render** separate in your explanation. Setup creates the ingredients; update changes the world; render displays its current state.

## 4. A mesh is numbers organised into shapes

A mesh contains vertex data and instructions for connecting it into shapes. A vertex position says where a corner is. A normal says which direction the surface faces, which we need for lighting. Texture coordinates describe a location on a potential texture; our ring currently calculates them but does not use a texture.

OpenGL can draw triangles and lines. A round-looking torus is a lot of small flat triangles approximating a curved surface. **Tessellation here is our CPU calculation of a triangle mesh. We do not use a tessellation-control or tessellation-evaluation shader.**

There are two useful ways to supply vertex data:

- **Indexed:** store vertices once and list their indices in triangle order. Our torus does this because neighbouring triangles share vertices.
- **Non-indexed:** read vertices in their stored order. With `GL_LINES`, every pair makes a line segment. Our crosshair uses this because four endpoints are enough.

### Build the easy mesh first: the crosshair

Open [createCrosshair](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:23). The function returns `bonobo::mesh_data`, which contains OpenGL object identifiers and mesh counts.

```cpp
std::array<glm::vec3, 4> const vertices = {{
    glm::vec3(-0.03f,  0.00f, 0.0f),
    glm::vec3( 0.03f,  0.00f, 0.0f),
    glm::vec3( 0.00f, -0.03f, 0.0f),
    glm::vec3( 0.00f,  0.03f, 0.0f)
}};
```

The declaration means “a fixed array of four three-dimensional positions that we will not modify.” The first two positions have the same Y and Z but different X, so they form a horizontal segment. The last two have the same X and Z but different Y, so they form a vertical segment. Every Z is zero in the mesh's local coordinates. Together, the segments make a `+` centred at local `(0,0,0)`.

The buffer setup is worth understanding line by line:

```cpp
bonobo::mesh_data data;
data.name = "Crosshair";
data.drawing_mode = GL_LINES;
data.vertices_nb = static_cast<GLsizei>(vertices.size());
glGenVertexArrays(1, &data.vao);
glBindVertexArray(data.vao);
glGenBuffers(1, &data.bo);
glBindBuffer(GL_ARRAY_BUFFER, data.bo);
glBufferData(GL_ARRAY_BUFFER,
             static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3)),
             vertices.data(), GL_STATIC_DRAW);
glEnableVertexAttribArray(0u);
glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
glBindVertexArray(0u);
glBindBuffer(GL_ARRAY_BUFFER, 0u);
return data;
```

| Code | Meaning |
| --- | --- |
| `mesh_data data` | Creates a structure to return the mesh's identifiers/counts. |
| `data.name` | Gives the mesh a debugging name. |
| `data.drawing_mode = GL_LINES` | Makes OpenGL connect vertices in separate pairs: 0–1, then 2–3. |
| `data.vertices_nb` | Stores 4, explicitly converted to the integer type OpenGL expects. |
| `glGenVertexArrays` | Obtains one vertex-array object, or VAO. `&data.vao` is where its identifier is written. |
| `glBindVertexArray` | Selects that VAO so subsequent attribute setup is recorded in it. A VAO describes how to read vertex buffers; it is not itself the array of vertex coordinates. |
| `glGenBuffers` | Obtains a buffer object to hold the vertex data on the graphics side. |
| `glBindBuffer(GL_ARRAY_BUFFER, ...)` | Selects that buffer as the destination for vertex-data operations. |
| `glBufferData` first argument | Says which bound buffer target to allocate/fill. |
| `vertices.size() * sizeof(glm::vec3)` | Calculates the required number of bytes: number of elements times bytes per element. |
| `vertices.data()` | Supplies a pointer to the first element to copy. |
| `GL_STATIC_DRAW` | A usage hint: upload this data once and draw from it repeatedly. |
| `glEnableVertexAttribArray(0u)` | Enables vertex attribute location 0, the position input. |
| `glVertexAttribPointer(0u, 3, GL_FLOAT, GL_FALSE, 0, nullptr)` | Describes attribute 0 as three floats, no integer normalisation, tightly packed, starting at buffer offset zero. The VAO remembers the currently bound buffer for this attribute. |
| `glBindVertexArray(0u)` | Stops editing this VAO. |
| `glBindBuffer(..., 0u)` | Unbinds the current array buffer. It does not delete the uploaded data. |
| `return data` | Gives the caller the identifiers needed to render and later delete the mesh. |

No normal attribute is needed for the crosshair because its fragment shader outputs solid white and does not calculate lighting. No index buffer is created. `Node::render` therefore uses `glDrawArrays`, drawing two lines from the four positions because we selected `GL_LINES`.

## 5. Make the torus using two circles

Open [createTorus, line 328](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/parametric_shapes.cpp:328). A torus can be imagined as a small circle swept around a larger circle.

- Major radius `R = 2`: centre of the whole ring to centre of its tube.
- Minor radius `r = 0.25`: radius of the tube itself.
- `theta` goes around the large circle.
- `phi` goes around the tube's circular cross-section.

Both angles cover `0` to `2π` radians. `π` radians is 180°, and `2π` radians is 360°.

```text
distance = R + r cos(phi)
x = distance cos(theta)
y = distance sin(theta)
z = r sin(phi)
```

The first line chooses how far this surface point sits from the Z axis. The next two place it around the main circle. The last line lifts it forward or backward relative to the central XY plane.

Worked points with `R=2`, `r=0.25`:

| Angles | Position | Interpretation |
| --- | --- | --- |
| `theta=0`, `phi=0` | `(2.25, 0, 0)` | Outer edge on the right. |
| `theta=0`, `phi=π` | `(1.75, 0, 0)` | Inner edge on the right. |
| `theta=π/2`, `phi=0` | `(0, 2.25, 0)` | Outer edge at the top. |
| `theta=0`, `phi=π/2` | `(2, 0, 0.25)` | Front of the tube at the right. |

This also explains the opening radius: `R-r = 2-0.25 = 1.75`. The outside radius is `R+r = 2.25`.

### From continuous formula to a discrete mesh

The first `if` rejects invalid radii or too few segments. It returns an empty mesh instead of trying to build invalid geometry.

```cpp
unsigned int const major_segments = major_split_count + 1u;
unsigned int const minor_segments = minor_split_count + 1u;
unsigned int const rows = major_segments + 1u;
unsigned int const cols = minor_segments + 1u;
```

Our course function's convention makes `63` splits become `64` major segments and `15` splits become `16` minor segments. Another point is needed at the end of each closed circle, so the grid has 65 rows and 17 columns. The last row/column repeats the first spatial position. This closes the seam and allows texture coordinates 0 and 1 to be stored separately.

```cpp
auto vertices = std::vector<glm::vec3>(rows * cols);
auto normals = std::vector<glm::vec3>(rows * cols);
auto texcoords = std::vector<glm::vec3>(rows * cols);
```

Each line allocates `65 × 17 = 1105` entries of one kind. These CPU vectors are temporary working arrays used to build and upload the mesh. The fixed-object-array requirement concerns our eight ring Nodes; using vectors during mesh generation does not change that.

Inside the nested loops:

```cpp
float const u = static_cast<float>(i) / static_cast<float>(major_segments);
float const theta = glm::two_pi<float>() * u;
float const v = static_cast<float>(j) / static_cast<float>(minor_segments);
float const phi = glm::two_pi<float>() * v;
float const distance = major_radius + minor_radius * std::cos(phi);
auto const index = i * cols + j;
```

`i` chooses a row around the large circle. Dividing by 64 converts it to a fraction from 0 to 1; multiplying by `2π` converts that fraction to an angle. `j`, `v` and `phi` do the same for the tube. Casting to float makes the division fractional rather than integer division. `i * cols + j` converts a row/column address into one flat-array index. For row 2, column 3, the index is `2 × 17 + 3 = 37`.

The next assignment uses the torus formula to fill `vertices[index]`. The normal assignment is:

```cpp
normals[index] = glm::vec3(std::cos(phi) * std::cos(theta),
                         std::cos(phi) * std::sin(theta),
                         std::sin(phi));
texcoords[index] = glm::vec3(u, v, 0.0f);
```

The normal points from the tube's centreline out to its surface. At the outer right edge it is `(1,0,0)`; at the inner right edge it is `(-1,0,0)`. Its length is 1 because `cos²(theta)+sin²(theta)=1` and `cos²(phi)+sin²(phi)=1`. Texture coordinates simply store the two fractions; our current shaders do not read them.

### Turn each grid cell into two triangles

```cpp
unsigned int const a = i * cols + j;
unsigned int const b = (i + 1u) * cols + j;
unsigned int const c = b + 1u;
unsigned int const d = a + 1u;
triangles.emplace_back(a, b, c);
triangles.emplace_back(a, c, d);
```

`a` is the current corner, `b` is the next row, `c` is the next row and next column, and `d` is the next column of this row. The last two lines append two triples of indices. They split the four-corner cell along the `a–c` diagonal. For the first cell, the indices are `(0,17,18)` and `(0,18,1)`. Their order gives outward-facing triangles.

There are `64 × 16 = 1024` cells and two triangles per cell, so **2048 triangles**. Each triangle has three indices, so **6144 indices**. A triangle count is different from a vertex count: adjacent triangles reuse positions.

`triangles.reserve(...)` preallocates capacity for these triples; it does not create triangles. Each `emplace_back(...)` appends one actual triple.

### Upload the torus

The VAO/VBO setup follows the crosshair buffer pattern, with three arrays instead of just positions. See [line 384](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/parametric_shapes.cpp:384).

```cpp
auto const array_bytes = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
glBufferData(GL_ARRAY_BUFFER, 3 * array_bytes, nullptr, GL_STATIC_DRAW);
glBufferSubData(GL_ARRAY_BUFFER, 0, array_bytes, vertices.data());
glBufferSubData(GL_ARRAY_BUFFER, array_bytes, array_bytes, normals.data());
glBufferSubData(GL_ARRAY_BUFFER, 2 * array_bytes, array_bytes, texcoords.data());
```

The first line finds the size of one array. The second reserves three arrays' worth of bytes; `nullptr` means there is no initial source data in that call. The last three copy positions, then normals, then UVs into consecutive blocks. They are **not interleaved** position/normal/UV per vertex.

Attribute locations 0, 1 and 2 are enabled. Their `glVertexAttribPointer` calls all describe three floats per entry, with offsets `0`, `array_bytes` and `2 * array_bytes`. The `reinterpret_cast<void const*>(...)` is the OpenGL API's way of expressing a byte offset in a bound buffer; we do not dereference that value as a CPU address.

```cpp
glGenBuffers(1, &data.ibo);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
glBufferData(GL_ELEMENT_ARRAY_BUFFER,
             static_cast<GLsizeiptr>(triangles.size() * sizeof(glm::uvec3)),
             triangles.data(), GL_STATIC_DRAW);
```

These lines create, select and fill the index buffer. Each `uvec3` holds three unsigned vertex indices. The VAO remembers its element-buffer binding. We unbind the VAO first at the end, then unbind the remaining buffer targets, and return the mesh data.

**Check:** If there were 8 major segments and 4 minor segments, how many triangles? **Answer:** `8 × 4 × 2 = 64`.

## 6. One mesh can produce eight objects

At [line 131](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:131), `std::array<Node, 8> rings;` creates exactly eight Nodes. A Node stores how to draw an object and its transform; it is not a second copy of all the mesh vertices.

```cpp
auto& ring = rings[i];
ring.set_geometry(ring_mesh);
ring.set_program(&ring_shader, [](GLuint program) {
    auto const light_position = glm::vec3(-3.0f, 4.0f, 6.0f);
    glUniform3fv(glGetUniformLocation(program, "light_position"),
                 1, glm::value_ptr(light_position));
});
```

`auto&` refers to the actual array element, so changes affect it. `set_geometry` associates the shared mesh identifiers with that Node. `&ring_shader` gives the Node the shader program variable's address, allowing shader reload to replace the program identifier. The lambda `[](GLuint program) { ... }` is a small callback function. Node calls it after activating the shader. Inside it, we define the light position, find the shader uniform named `light_position`, and upload one vector of three floats. `glm::value_ptr` provides access to those float values in the format OpenGL expects.

All eight Nodes share mesh and shader but have independent position and rotation. The ring-offset array gives their X/Y path: centre, right, upper right, up, upper left, left, lower left, down.

Initial Z uses:

```cpp
float const z = ring_start_z - ring_spacing * static_cast<float>(i);
```

With start `-12` and spacing `6`, the values are `-12,-18,-24,-30,-36,-42,-48,-54`. This expression runs inside the restart block, so startup and Restart use the same placement code.

## 7. Model, view and projection: how 3D numbers reach the screen

There are three transformations to keep separate:

1. **Model matrix M:** local object coordinates → world coordinates. Each Node provides its translation, rotation and scale.
2. **View matrix V:** world coordinates → camera coordinates. It is the inverse of the camera's world transform.
3. **Projection matrix P:** camera coordinates → clip coordinates. It makes nearer objects appear larger.

Our vertex shader writes:

```cpp
gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
```

This means `clip_position = P × V × M × local_position`. Operations act from right to left. `GetWorldToClipMatrix()` supplies `P × V`. Our framework builds the ordinary object matrix as `T × R × S`, so scale acts first, then rotation, then translation.

Example: an unrotated ring at world `(0,0,-12)` has a local point `(2.25,0,0)`. Model transformation places that point at `(2.25,0,-12)`. With camera `(0,0,8)`, the view transform makes it `(2.25,0,-20)`: twenty units in front of the camera.

For our 45° vertical field of view, `f = 1 / tan(22.5°) ≈ 2.414`. Ignoring depth for this small illustration, after perspective division:

```text
x_ndc = f * x_view / (aspect * -z_view)
y_ndc = f * y_view / -z_view
```

NDC means normalised device coordinates; visible horizontal/vertical bounds are approximately -1 to +1. At aspect `16/9`, a point at view `(2,0,-20)` has `x_ndc ≈ 0.136`. At `(2,0,-10)` it has `x_ndc ≈ 0.272`: half the distance makes the screen offset twice as large. The GPU performs the division by clip `w` after the vertex shader. We rely on GLM/the framework for the full projection matrix.

Why `vec4(vertex, 1.0)`? A fourth coordinate of 1 represents a point, so translation applies. For a direction such as a normal we use 0, so translation does not apply.

### Why the crosshair stays at the centre of the screen

At [the crosshair transform](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:128), we give it local translation `(0,0,-2.5)`. Its X and Y remain zero, so it sits straight ahead. Negative Z places it 2.5 units in front of the camera. We leave its scale at the default 1 and do not rotate it.

```cpp
crosshair.render(mCamera.GetWorldToClipMatrix(), mCamera.GetViewToWorldMatrix());
```

The second argument is the parent transform. Node multiplies it by the crosshair's own transform, just like using a parent/child transformation in the solar-system lab. Let the camera world transform be `C`, so `V = C⁻¹`. Then:

```text
crosshair clip position = P × C⁻¹ × C × crosshair_local_transform × vertex
                        = P × crosshair_local_transform × vertex
```

The camera movement cancels. The crosshair stays at the screen centre while the ring view changes. We do not add it to a stored child list; passing the parent matrix directly provides the hierarchy for this draw call.

Each line is `0.03 - (-0.03) = 0.06` local units long. With our 45° field of view and a 900-pixel-high viewport, its projected length is approximately `900/2 × 2.414 × 0.06/2.5 = 26` pixels. It stays small because its size and distance relative to the camera stay fixed.

## 8. Time and input: why movement uses dt

Read [the frame loop, line 177](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:177). Each trip around `while` produces one frame.

```cpp
auto const nowTime = std::chrono::steady_clock::now();
auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
lastTime = nowTime;
float const dt = glm::min(std::chrono::duration<float>(deltaTimeUs).count(), 0.1f);
```

The first line reads the clock; the second subtracts the previous time and stores microseconds; the third remembers the current time for next frame; the fourth converts the duration to seconds and caps it at `0.1`. `steady_clock` is appropriate for elapsed time because it is not meant to jump when wall-clock settings change.

Movement is **distance = speed × time**:

```cpp
position.z += ring_speed * dt;
ring.get_transform().RotateZ(spin_speed * dt);
```

At 60 frames/second, `dt ≈ 1/60`. A ring moving at 4 units/second advances `4/60 ≈ 0.0667` units each frame, or about 4 units in 60 frames. At 30 frames/second it advances `4/30 ≈ 0.1333` each frame and still covers 4 units in a second.

Rotation uses the same idea: `0.8 radians/second × dt`. A full revolution takes `2π/0.8 ≈ 7.85 seconds`. Radians are the angular units used by the transform functions.

The `0.1` cap is a practical game choice: a debugger stop or stalled frame cannot make the scene jump several seconds forward. After such a stall, simulated time deliberately advances less than real time. This is simple variable-time animation, not a fixed-step physics simulation.

### Steering, one line at a time

```cpp
glm::vec3 direction(0.0f);
if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) direction.x -= 1.0f;
if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) direction.x += 1.0f;
if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) direction.y += 1.0f;
if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) direction.y -= 1.0f;
```

The first line starts at `(0,0,0)`. Each remaining line adds a component while its key is held. A and D together cancel, as do W and S. These lines are inside a UI-capture check so typing into an ImGui control takes priority over steering.

`& PRESSED` checks a bit in the key-state flags. This is different from the address operator in `&ring_speed` or the reference symbol in `auto& ring`. C++ uses the same symbol for different operations depending on context.

```cpp
if (glm::length(direction) > 0.0f) {
    camera_position += glm::normalize(direction) * steering_speed * dt;
}
```

`length` computes vector magnitude. `normalize` divides a nonzero vector by its length, preserving direction but making its length 1. We check for zero first because a zero vector has no direction to normalise.

Holding D gives `(1,0,0)`, length 1. Holding D and W gives `(1,1,0)`, length `√2 ≈ 1.414`. Without normalisation, diagonal movement would be about 41% faster. After normalisation, `(1,1,0)` becomes approximately `(0.707,0.707,0)`.

With steering speed 4 and `dt=0.1`, diagonal movement is `(0.2828,0.2828,0)`. Its total distance is `√(0.2828²+0.2828²) = 0.4`, exactly the same total distance as a straight move.

The two `glm::clamp` calls keep camera X and Y in `[-4,4]`. For example, a proposed X of 4.2 becomes 4.0. `SetTranslate(camera_position)` then applies the resulting position; it does not turn the camera towards the world origin again.

**Check:** At speed 4 and `dt=0.025`, how far does a ring move? **Answer:** `4 × 0.025 = 0.1` units.

## 9. Decide whether a ring was passed

Our player is treated as a **point at the camera position**. The white crosshair marks the viewing direction through the screen centre. Keep it inside the approaching opening. The scoring calculation uses the camera point, not the drawn endpoints of the crosshair; we do not simulate bouncing or damage.

Read [the crossing test, line 258](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:258):

```cpp
if (previous_z < camera_position.z && position.z >= camera_position.z) {
```

Before updating, the ring's centre must be in front of camera Z=8. After updating, it must have reached or passed Z=8. Both conditions together identify the frame that contains the crossing. This avoids giving a point every frame while a ring is nearby.

### Find where the player was at the crossing instant

```cpp
float const crossing_fraction = (camera_position.z - previous_z) / (position.z - previous_z);
auto const crossing_position = previous_camera_position
    + crossing_fraction * (camera_position - previous_camera_position);
```

The first line finds the fraction of this frame's Z movement required to reach camera Z. If a ring moves from 7.9 to 8.1, then:

```text
t = (8 - 7.9) / (8.1 - 7.9) = 0.1 / 0.2 = 0.5
```

The ring crossed halfway through the frame. If the camera moved from X=0 to X=0.4 during that frame, its crossing X is:

```text
x = old_x + t(new_x - old_x) = 0 + 0.5(0.4 - 0) = 0.2
```

This is **linear interpolation**, also called lerp: `old + t × (new-old)`. It assumes the update travels linearly between its two endpoints. It is a small estimate used for this scoring rule, not a full continuous collision solver. The crossing condition guarantees positive Z movement here, so its denominator is not zero.

### Measure distance in the opening plane

```cpp
float const distance = glm::length(glm::vec2(crossing_position - position));
if (distance < opening_radius) {
    ++score;
    last_result = "Through the opening! +1 point";
} else {
    ++missed_rings;
    last_result = "Missed. Aim for the next opening.";
}
```

Subtracting positions gives the camera's displacement from the ring centre. Constructing `vec2` discards Z, retaining X/Y. `length` uses Pythagoras:

```text
d = sqrt(dx² + dy²)
```

Suppose the ring centre is `(2,0)` and the camera's crossing position is `(1,1)`. Then `dx=-1`, `dy=1`, so `d=√2≈1.414`. Since `1.414 < 1.75`, it passes. At `(0,0)` the distance from the same ring centre is 2, which misses. Exactly 1.75 also misses because the code uses strict `<`.

`++` increases a counter by one. `last_result` is a pointer to one of our fixed text messages; assigning it chooses the message the UI will display.

This is an **opening-crossing rule**, not a full torus collision test. It stays simple because the rings only spin around Z; their circular openings remain in the XY plane. Tilting them about X or Y, scaling them differently, or giving the player a physical size would require reconsidering the test. None of those complications are needed for our present game.

## 10. Reuse eight rings to make an endless course

The fixed array never grows during play. After a ring's centre passes behind the camera to Z=10, we move it back by `8 × 6 = 48` units:

```cpp
while (position.z >= ring_end_z) {
    position.z -= loop_length;
}
ring.get_transform().SetTranslate(position);
```

For example, Z=10.2 becomes `10.2 - 48 = -37.8`. Keeping the extra 0.2 preserves movement continuity instead of losing part of the frame's motion. Its X/Y path offset remains unchanged. The code uses `while` so repeated subtraction can handle more than one loop-length overshoot, although the current speed range and time cap make that unnecessary in normal play.

At default speed 4, the interval between neighbouring rings is `6/4 = 1.5` seconds. An individual ring takes `48/4 = 12` seconds to complete a cycle. Ring 0 initially crosses at about five seconds, ring 1 at 6.5, and so on. The path repeats using the same eight objects.

We check scoring **before** recycling. Otherwise, moving a ring back could confuse which crossing belongs to the forward movement.

**Check:** A ring is at Z=10.05. What is its new Z after one recycle? **Answer:** `-37.95`.

## 11. The shaders turn the geometry into a lit picture

The CPU runs our C++ update and issues draw calls. The GPU runs the shaders for the geometry being drawn.

### Vertex shader

Open [neon_ring.vert](/Users/sugatasarker/Developer/CG_Labs/shaders/EDAF80/neon_ring.vert:1).

`#version 410` selects GLSL 4.10. `layout(location=0) in vec3 vertex` receives the position attribute from the VAO; location 1 receives its normal. A `uniform` is a value supplied for a draw, such as a matrix or light position. An `out` value from the vertex shader is passed through the rasterisation pipeline to the fragment shader, normally interpolated across the triangle.

```glsl
vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
vs_out.normal = vec3(normal_model_to_world * vec4(normal, 0.0));
vs_out.local_vertex = vertex;
gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(vertex, 1.0);
```

Line 1 produces a world-space surface position for lighting. Line 2 transforms its normal into world space. Node supplies the normal matrix as the inverse transpose of the model matrix; this preserves the normal's perpendicular relationship to the surface even under nonuniform scaling. Our rings currently use rotation and translation, but the framework provides the general solution. Line 3 preserves the original model-space position for the orange marking. Line 4 produces clip-space position for rendering.

### Fragment shader and diffuse lighting

Open [neon_ring.frag](/Users/sugatasarker/Developer/CG_Labs/shaders/EDAF80/neon_ring.frag:1). Its `in VS_OUT` block matches the vertex shader's `out VS_OUT` fields. A fragment is a candidate contribution to a pixel or sample; the depth test and other pipeline rules determine whether it is used.

```glsl
vec3 N = normalize(fs_in.normal);
vec3 L = normalize(light_position - fs_in.vertex);
float diffuse = max(dot(N, L), 0.0);
```

Line 1 makes the interpolated normal unit length again. Line 2 subtracts the surface position from the light position to obtain a direction towards the light, then normalises it. Line 3 uses the dot product. For unit vectors, `dot(N,L)=cos(angle)`.

| Surface/light angle | Dot product | After `max(...,0)` |
| --- | --- | --- |
| 0°: directly facing the light | 1 | 1: brightest diffuse term. |
| 60° | 0.5 | 0.5. |
| 90°: side-on | 0 | 0. |
| 180°: facing away | -1 | 0: no negative light. |

The colour code is:

```glsl
vec3 ring_colour = vec3(0.05, 0.8, 1.0);
if (fs_in.local_vertex.y > 1.4) {
    ring_colour = vec3(1.0, 0.45, 0.08);
}
frag_colour = vec4(ring_colour * (0.18 + 0.82 * diffuse), 1.0);
```

The first line chooses cyan as RGB. The `if` selects the top part of the original, untransformed ring and makes it orange. The final line multiplies colour by ambient-plus-diffuse brightness and gives it alpha 1, meaning opaque.

With diffuse 0, brightness is 0.18, so the unlit side stays visible. With diffuse 0.5, it is `0.18 + 0.82×0.5 = 0.59`. With diffuse 1, it is 1. This is a simple ambient-plus-Lambert lighting model; it has no shadows or specular highlights.

**Why does orange rotate?** We choose its colour using the model-space position before rotation, so the selected surface region stays attached to the ring as the model matrix turns it. If we chose based on world Y instead, the selected region would follow a fixed world-space boundary. A uniform cyan torus spinning about its symmetry axis would be hard to distinguish from a stationary one, so the orange segment makes the required rotation easy to see.

### Crosshair shaders

The course [fallback vertex shader](/Users/sugatasarker/Developer/CG_Labs/shaders/common/fallback.vert:1) uses the same model/view/projection position calculation. The [fallback fragment shader](/Users/sugatasarker/Developer/CG_Labs/shaders/common/fallback.frag:1) writes `vec4(1.0)`, which expands to `(1,1,1,1)`: opaque white. That is why no crosshair normals, lights or textures are needed.

## 12. Rendering, controls and cleanup

At [line 281](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:281), the update is finished and drawing begins.

```cpp
mWindowManager.NewImGuiFrame();
glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
```

The first call starts the UI frame. The second clears both the old picture and old depth values. The earlier `glClearColor` selects the dark background; `glClearDepthf(1.0f)` selects the cleared depth value. The depth buffer normally lets nearer fragments hide farther fragments.

```cpp
bonobo::changePolygonMode(polygon_mode);
for (auto const& ring : rings) {
    ring.render(mCamera.GetWorldToClipMatrix());
}
glDisable(GL_DEPTH_TEST);
crosshair.render(mCamera.GetWorldToClipMatrix(), mCamera.GetViewToWorldMatrix());
glEnable(GL_DEPTH_TEST);
```

We select Fill, Line or Point, then draw each ring with its own transform and the same camera matrix. `Node::render` activates the shader, uploads matrices and the callback's light uniform, binds the VAO, and issues the draw call. It uses `glDrawElements` for the indexed torus and `glDrawArrays` for the non-indexed crosshair.

The crosshair is drawn last with depth testing temporarily disabled so an approaching ring cannot hide the aiming marker. We restore depth testing immediately afterwards. This makes it a visual overlay. `GL_LINES` draws line segments directly, so the crosshair stays a `+` in Fill, Line and Point modes; polygon mode changes the torus triangles.

`glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)` restores filled rendering before drawing the interface. Otherwise a debug wireframe setting could interfere with UI rendering.

### UI and restart

The ImGui calls at [line 301](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:301) display instructions, counters and the last result. `Begin` starts the window; its return value tells us whether to add the visible contents; `End` is still called even when the window is collapsed.

`Button("Restart")` returns true on a click, setting `restart_requested`. The restart block on the next frame resets camera position, counters, feedback, pause state, and all ring positions/rotations. Setting the request false consumes it. It does not reconstruct the meshes. Speed and debug-display choices are left at their selected settings.

`Checkbox` and `SliderFloat` receive addresses such as `&pause_motion` and `&ring_speed` so ImGui can modify those actual variables. The speed slider stays within 1–10. The polygon dropdown uses the same course helper as the earlier labs.

The rest of the retained framework code handles F2 for showing/hiding the UI, F3 for logs, F11 for fullscreen, and R for shader reload. Shader-reload failure suspends scene drawing and gameplay updates until a successful reload; it also reports the error. Framebuffer-size queries and `glViewport` ensure drawing uses the actual pixel size, including high-DPI displays.

`RenderImGuiFrame(show_gui)` submits the interface. `glfwSwapBuffers(window)` displays the completed back buffer. The loop then starts the next frame.

### Resource cleanup

After the loop, [line 326](/Users/sugatasarker/Developer/CG_Labs/src/EDAF80/assignment5.cpp:326) deletes the crosshair vertex buffer and VAO, then the torus index buffer, vertex buffer and VAO. The torus GPU objects are deleted **once**, even though eight Nodes refer to them. Deleting them once per ring would be incorrect shared-resource handling. The crosshair has no index buffer to delete. The shader manager and other framework objects handle their own lifetimes.

## 13. C++ notation to be able to explain

| Notation | Meaning in this game |
| --- | --- |
| `float`, `4.0f` | A floating-point value and a float literal. Used for positions, speeds and time. |
| `int` | Whole-number counters such as score. |
| `bool` | True/false settings such as pause. |
| `const` | This value cannot be changed through this declaration after initialisation. |
| `auto` | Let the compiler infer the type from the expression. |
| `auto& ring` | A reference to the existing ring, so updates affect the array element. |
| `auto const& ring` | A reference used for reading/drawing without changing that ring through it. |
| `std::array<T,N>` | A fixed count N of values of type T. |
| `std::vector<T>` | A runtime-sized sequence used while constructing mesh data. |
| `std::size_t`, `0u` | An array-size/index type and an unsigned integer literal. |
| `static_cast<float>(i)` | An explicit numeric conversion, useful before fractional arithmetic. |
| `+=`, `-=`, `++` | Add to, subtract from, or increase by one. |
| `!`, `&&`, `||` | Not, both conditions, or at least one condition. |
| `nullptr` | A null pointer, such as a missing window or no initial buffer data. |
| `[](...) { ... }` | A lambda: a small function we pass to the Node as a shader-uniform callback. |
| `glm::vec2(vec3_value)` | Keep the first two components, discarding Z. |
| `char const*` | A pointer to fixed text; we switch which message is selected. |
| `GLuint` | OpenGL's unsigned integer type, often used for object identifiers. |

## 14. What maths we actually used

| Maths | Actual use |
| --- | --- |
| Sine/cosine and radians | Torus positions/normals; rotation angles through the transform library. |
| Vector subtraction | Relative positions and direction to the light. |
| Pythagoras / vector length | Opening test and movement normalisation. |
| Unit vectors | Equal-speed steering and diffuse lighting. |
| Speed × elapsed time | Ring translation, camera steering and ring rotation. |
| Linear interpolation | Player position at the ring's crossing instant. |
| Matrix multiplication and inverse | Model/view/projection and camera-relative crosshair. |
| Inverse transpose | Framework transforms surface normals correctly. |
| Dot product | Lambert diffuse brightness. |
| Simple bounds and comparisons | Pause/restart logic, input checks, clipping steering range and recycling. |

We did not add acceleration, gravity, a physics engine, ray casting, full torus collisions, randomness, path splines, or a complex state machine. The game-specific choices are the fixed path, colour marker, point-camera opening rule, time cap and aiming crosshair. They are combinations of basic course tools; they are not new required theory you must pretend was copied from a slide.

## 15. Practise explaining it to the TA

Use this short account as a starting point, then explain each sentence in your own words:

> I made the Torus Ride suggested in the seminar using the course framework. A torus mesh is generated from two circle angles and split into triangles. Eight Nodes share that mesh but have separate transforms. WASD moves the camera in the XY plane, and the rings move towards it using speed times frame time. They rotate about Z; a model-space orange marking makes that visible. When a ring crosses camera Z, I check the XY distance from its centre and update the score or misses. Once a ring is behind us, I reuse it at the back of the course. The crosshair is two line segments drawn relative to the camera, so it stays at the screen centre. ImGui provides feedback and the controls.

Likely follow-up questions:

**Why use an array?** The seminar asks for a fixed object array. Eight Nodes give a bounded, predictable scene; recycling provides repetition without creating objects during play.

**How many torus triangles?** `64 major segments × 16 minor segments × 2 = 2048`. There are 1105 stored vertices including duplicated seams, and 6144 indices.

**Why use indices?** Neighbouring triangles reuse vertex data. The index buffer specifies connectivity without repeating all position/normal/UV data for every triangle.

**Why does the ring appear to move forward if the camera does not move along Z?** The visible result depends on relative positions. Moving rings towards a fixed-Z camera creates that impression.

**Why use dt?** It converts a speed per second into a distance or angle for this particular frame, so normal frame-rate variation does not set the game's speed.

**Why normalise movement?** Otherwise holding two perpendicular direction keys produces a vector of length `√2` and moves faster.

**Is the orange part another object?** No. It is part of the same torus, coloured by the fragment shader using the original vertex position.

**Is this physical collision detection?** No. It is a point passing through a circular opening at one plane. It gives a simple playable scoring rule.

**Why interpolate the crossing position?** A ring can cross halfway through a frame. We estimate where the player was at that moment instead of automatically using the end-of-frame position.

**Does the crosshair change scoring?** No. The camera point is still the player for scoring. The crosshair makes its viewing direction visible using a parent camera transform.

**Why disable depth for the crosshair?** It is an aiming marker that should remain visible. The ring scene still uses depth testing, and we restore it after drawing the crosshair.

**Why does the crosshair not drift when the camera moves?** Its world matrix includes the camera's world transform; the inverse camera transform in the view matrix cancels it.

**Why is it white?** We reused the supplied fallback fragment shader, which returns `(1,1,1,1)`.

**What happens when you release A or D?** The direction X returns to zero, so horizontal movement stops on the next active update. There is no inertia, and the crosshair stays centred.

**What do Line and Point modes demonstrate?** The same geometry is rendered with triangle edges or points instead of filled triangles. Line mode makes tessellation easy to show.

**What did the framework supply?** Window/input/UI integration, camera and transform classes, Node rendering, shader management and common helpers. Our additions are geometry generation, ring shader modifications and the game-specific setup/update logic.

**What remains for submission?** Follow the lab presentation and forum-gallery instructions, add your creator details and media, and check the actual Canvas submission description. The code does not submit itself.

Final self-check: without looking at code, explain the difference between a mesh and a Node; compute one torus point; predict movement for a given dt; explain model/view/projection; decide a pass from two XY positions; and explain why the orange segment and crosshair behave as they do. If you can do those, you understand the central design rather than only memorising its names.
