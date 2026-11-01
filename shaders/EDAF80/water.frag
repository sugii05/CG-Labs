#version 410

uniform vec3 camera_position;
uniform samplerCube cubemap;
uniform sampler2D normal_map;
uniform float elapsed_time_s;




in VS_OUT {
	vec3 vertex;
	vec3 normal;
    vec2 uv;
    vec3 tangent;

} fs_in;

out vec4 frag_color;

void main()
{
    vec3 N = normalize(fs_in.normal);
    vec3 T = normalize(fs_in.tangent);
    vec3 B = normalize(cross(T, N));
    vec2 ripple_uv = fs_in.uv * 8.0 + elapsed_time_s * vec2(0.02, 0.01);
    mat3 TBN = mat3(T, B, N);
    vec2 texScale = vec2(8.0, 4.0);
    vec2 normalSpeed = vec2(-0.05, 0.0);
    float normalTime = mod(elapsed_time_s, 100.0);

    vec2 uv0 = fs_in.uv *texScale + normalTime * normalSpeed;
    vec2 uv1 = fs_in.uv *texScale * 2.0 + normalTime * normalSpeed * 4.0;
    vec2 uv2 = fs_in.uv *texScale * 4.0 + normalTime * normalSpeed * 8.0;


    vec3 n0 = texture(normal_map, uv0).rgb * 2.0 - 1.0;
    vec3 n1 = texture(normal_map, uv1).rgb * 2.0 - 1.0;
    vec3 n2 = texture(normal_map, uv2).rgb * 2.0 - 1.0;
    vec3 map_normal = normalize(n0 + n1 + n2);

    

   
    N = normalize(TBN * map_normal);


    //frag_color = vec4(reflect_colour, 1.0);
    //float frensel = 0.02 + 0.98 * pow(1.0 - facing, 5.0);
    //vec3 final_colour = mix(colour, reflect_colour, frensel);
    //frag_color = vec4(final_colour, 1.0);
    
    vec3 V = normalize(camera_position - fs_in.vertex);
    float facing = clamp(dot(N, V), 0.0, 1.0);

    vec3 grazing_colour = vec3(0.0, 0.5, 0.5); 
    vec3 facing_colour = vec3(0.0, 0.0, 0.1);
    vec3 colour = mix(grazing_colour, facing_colour, facing);
    vec3 R = reflect(-V, N);
    vec3 reflect_colour = texture(cubemap, R).rgb;
    vec3 refraction_direction = refract(-V, N, 1.0/1.33);
    vec3 refract_colour = texture(cubemap, refraction_direction).rgb;
   
    float frensel = 0.02 + 0.98 * pow(1.0 - facing, 5.0);
    vec3 f_colour = colour + mix(refract_colour, reflect_colour, frensel);
    frag_color = vec4(f_colour, 1.0); 

}
