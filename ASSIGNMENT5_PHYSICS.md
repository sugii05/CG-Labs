# Neon Run: understand the new physics

WASD now applies acceleration. Release the keys to coast and gradually slow down. Hold Space to brake; Space takes priority over WASD. Your camera stays at Z = 8, and the rings still move towards you independently. The default ring speed remains 4 units/second.

This follows **Seminar 5, printed slide 15, “Physics: acceleration / inertia”**: update velocity using acceleration, then update position using velocity. We use the existing GLM vectors and C++ loops. No physics engine or extra library is needed.

## The three quantities

- **Position:** where the camera is, in world units.
- **Velocity:** how fast and in which direction it moves, in units/second. Stored between frames in `camera_velocity`.
- **Acceleration:** how quickly velocity changes, in units/second².

The main code in `src/EDAF80/assignment5.cpp` is:

```cpp
glm::vec3 const acceleration = direction * steering_acceleration
    - camera_velocity * active_drag;
camera_velocity += acceleration * step;
camera_position += camera_velocity * step;
```

The actual implementation limits velocity before updating position. This update order is called semi-implicit Euler integration: use the new velocity to move. It is the same update order as the seminar example.

`direction` comes from WASD. D gives `(1,0,0)`, W gives `(0,1,0)`. We normalise combined directions so W+D does not give stronger acceleration. Space sets the direction to zero and selects stronger drag.

`direction * steering_acceleration` is the acceleration from the controls. `-camera_velocity * active_drag` opposes the current velocity, slowing the camera. For example, moving right gives positive X velocity, so drag adds negative X acceleration. This is a simple model of resistance, not contact friction against a surface. We treat mass as 1, so force and acceleration have the same numerical value. We do not simulate gravity in this flying game.

With velocity zero, D held, and one step of `1/120` second:

```text
acceleration.x = 18 - 3 × 0 = 18
velocity.x = 0 + 18 × (1/120) = 0.15 units/second
position.x increases by 0.15 × (1/120) = 0.00125 units
```

When you release D at velocity 4:

```text
acceleration.x = 0 - 3 × 4 = -12
velocity.x after one step = 4 - 12/120 = 3.9
```

You still move right, but more slowly. With Space, drag is 14 instead of 3, so the first step brings velocity from 4 to approximately 3.53. Braking slows lateral movement, not the approaching rings.

## The tuning numbers

| Variable | Value | Purpose |
| --- | --- | --- |
| `steering_acceleration` | 18 | Builds speed promptly when holding WASD. |
| `maximum_steering_speed` | 4 | Caps the length of velocity, including diagonals. |
| `drag` | 3 | Gentle resistance, including when releasing the keys. |
| `brake_drag` | 14 | Stronger resistance while holding Space. |
| `maximum_physics_step` | 1/120 second | Keeps each physics update small. |
| Stopping threshold | 0.01 units/second | Removes tiny residual drift when no thrust is applied. |

These numbers are gameplay choices, not values prescribed by the course. The cap keeps steering manageable. Drag alone would approach a speed of `18/3 = 6` with continuous thrust, so we deliberately cap it at 4.

## Why the extra while loop?

```cpp
float remaining_time = dt;
while (remaining_time > 0.0f && !game_over) {
    float const step = glm::min(remaining_time, maximum_physics_step);
    remaining_time -= step;
```

A frame at 30 FPS lasts approximately `1/30` second. We split it into approximately four updates of `1/120` second; at 60 FPS there are approximately two. A short final step consumes any remainder. Both camera movement and ring movement happen inside this loop.

This is a small numerical-stability addition to the seminar example. It makes movement similar at different frame rates and keeps `1 - drag * step` positive, so drag does not accidentally reverse velocity. It is not a separate thread, and it is not a fixed-timestep accumulator. Very slow frames are still capped at 0.1 seconds, as before, so a long stall deliberately advances less than wall-clock time.

## Bounds, pause, restart and collision

- X and Y stay between -4 and 4. At a boundary we clear only outward velocity on that axis. You can immediately accelerate back inward or slide along the other axis.
- Pause and a shader reload failure freeze the physics updates. Pause keeps velocity, so movement resumes from the same state.
- Restart clears velocity, score, misses and game-over state, and restores all ring positions.
- A ring collision clears velocity and ends further physics steps. All rings finish the current small step together so their spacing stays consistent.
- Keyboard input captured by the UI does not apply thrust or Space braking. Existing momentum still decays under ordinary drag unless paused.

Collision still uses the existing **camera-point test when a ring crosses Z = 8**. We interpolate the camera's position at the crossing and measure its XY distance from the ring centre:

| Distance | Result |
| --- | --- |
| Less than 1.75 | Pass: add one point. |
| From 1.75 through 2.25 | Hit the tube: game over. |
| Greater than 2.25 | Miss: add one to the miss counter. |

This is an approximate crossing-plane collision rule, not full 3D torus collision or bounding-sphere physics. The ring can intersect the camera before its centre crosses the plane. Smaller physics steps improve motion integration but do not remove that geometric simplification. The crossing condition prevents a ring from scoring repeatedly over several frames.

## Explain it aloud

“Previously, pressing a key changed position directly at a fixed speed. Now keys provide acceleration. I store velocity between frames, update it using acceleration times time, and use velocity times time to move. Drag slows the camera when I release the keys. Space increases drag. I normalise input and cap speed to keep diagonals fair, and use small time steps for stable updates.”

Try explaining why releasing a key does not immediately stop the camera, why diagonal input needs normalisation, and why restart must reset velocity as well as position.
