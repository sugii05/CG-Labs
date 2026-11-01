#version 410

in vec3 lookup_direction;


uniform samplerCube cubemap;

out vec4 frag_color;

void main() {
    frag_color = texture(cubemap, lookup_direction);
}