#version 410 

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient_colour;
uniform vec3 diffuse_colour;
uniform vec3 specular_colour;
uniform float shininess_value;
uniform sampler2D diffuse_texture;
uniform sampler2D specular_texture;
uniform sampler2D normal_texture;
uniform bool use_normal_mapping;

in VS_OUT {
    vec3 vertex;
    vec3 normal;
    vec2 texcoord;
    vec3 tangent;
    vec3 binormal;
} fs_in;

out vec4 frag_colour;


void main() {
    vec3 N = normalize(fs_in.normal);
   

if (use_normal_mapping) {
   
    vec3 T = normalize(fs_in.tangent);
    
    //T = normalize(T - N * dot(T, N));

   
    vec3 B = normalize(cross(N, T));

  
    if (dot(B, fs_in.binormal) < 0.0) {
        B = -B;
    }

    mat3 TBN = mat3(T, B, N);

    vec3 mapped_normal =
        texture(normal_texture, fs_in.texcoord).rgb;

    mapped_normal = mapped_normal * 2.0 - 1.0;


    N = normalize(TBN * mapped_normal);
}
    vec3 L = normalize(light_position - fs_in.vertex);
    vec3 V = normalize(camera_position - fs_in.vertex);

    float diffuse_factor = max(dot(N, L), 0.0);
    float specular_factor = 0.0;

    if (diffuse_factor > 0.0) {
        vec3 R = reflect(-L,N);
        specular_factor = pow(max(dot(R, V), 0), shininess_value);
        
    };
    vec3 texture_colour = texture(diffuse_texture, fs_in.texcoord).rgb;
    vec3 specular_map_colour = texture(specular_texture, fs_in.texcoord).rgb;
    vec3 colour = ambient_colour + texture_colour*diffuse_colour*diffuse_factor + specular_map_colour*specular_colour * specular_factor;
    frag_colour = vec4(colour, 1.0);
    

}