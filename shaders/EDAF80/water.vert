#version 410

// Remember how we enabled vertex attributes in assignment 2 and attached some
// data to each of them, here we retrieve that data. Attribute 0 pointed to the
// vertices inside the OpenGL buffer object, so if we say that our input
// variable `vertex` is at location 0, which corresponds to attribute 0 of our
// vertex array, vertex will be effectively filled with vertices from our
// buffer.
// The surface normal is calculated from the wave slopes below.
layout (location = 0) in vec3 vertex;
layout(location = 2) in vec3 texcoord;


uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;
uniform float elapsed_time_s;

// This is the custom output of this shader. If you want to retrieve this data
// from another shader further down the pipeline, you need to declare the exact
// same structure as in (for input), with matching name for the structure
// members and matching structure type. Have a look at
// shaders/EDAF80/diffuse.frag.
out VS_OUT {
	vec3 vertex;
	vec3 normal;
    vec2 uv;
    vec3 tangent;
} vs_out;
// Return height and the two slopes (dG/dx, dG/dz).
float wave_height(vec2 position, vec2 direction, float frequency,
                  float amplitude, float phase, float sharpness, float time,
                  out vec2 slope)
{
    float theta = dot(direction, position) * frequency + time * phase;
    float alpha = 0.5 * sin(theta) + 0.5;
    float height = amplitude * pow(alpha, sharpness);

    // Chain rule: differentiate the power, sine, and phase in turn.
    float derivative = 0.5 * amplitude * sharpness * frequency
                     * pow(alpha, sharpness - 1.0) * cos(theta);
    slope = derivative * direction;
    return height;
}

void main()
{
    vec3 displaced = vertex;
    vec2 slope1;
    vec2 slope2;

    // Arguments: position, direction, frequency, amplitude,
    //            phase, sharpness, time, output slope.
    float h1 = wave_height(
        vertex.xz, vec2(-1.0, 0.0),
        0.2, 1.0, 0.5, 2.0, elapsed_time_s, slope1);

    float h2 = wave_height(
        vertex.xz, vec2(-0.7, 0.7),
        0.4, 0.5, 1.3, 2.0, elapsed_time_s, slope2);

    displaced.y = h1 + h2;
    vec2 slope = slope1 + slope2;
    vec3 surface_normal = normalize(vec3(-slope.x, 1.0, -slope.y));
    vec3 surface_tangent = vec3(1.0, slope.x, 0.0);

    vs_out.tangent = normalize(vec3(vertex_model_to_world* vec4(surface_tangent, 0.0)));
    vs_out.vertex = vec3(vertex_model_to_world * vec4(displaced, 1.0));
    vs_out.normal = normalize(vec3(normal_model_to_world * vec4(surface_normal, 0.0)));
    
    gl_Position = vertex_world_to_clip * vertex_model_to_world * vec4(displaced, 1.0);
    vs_out.uv = texcoord.xy;

}
