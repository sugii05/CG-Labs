#version 410

uniform vec3 light_position;
uniform vec3 camera_position;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 local_vertex;
} fs_in;

out vec4 frag_colour;

void main()
{
	vec3 N = normalize(fs_in.normal);
	vec3 L = normalize(light_position - fs_in.vertex);
	vec3 V = normalize(camera_position - fs_in.vertex);
	float diffuse = max(dot(N, L), 0.0);
	float specular = 0.0;
	if (diffuse > 0.0) {
		vec3 R = reflect(-L, N);
		specular = pow(max(dot(R, V), 0.0), 32.0);
	}
	vec3 ring_colour = vec3(0.05, 0.8, 1.0);
	if (fs_in.local_vertex.y > 1.4) {
		ring_colour = vec3(1.0, 0.45, 0.08);
	}
	vec3 colour = ring_colour * (0.20 + 0.80 * diffuse)
	            + vec3(0.35) * specular;
	frag_colour = vec4(colour, 1.0);
}
