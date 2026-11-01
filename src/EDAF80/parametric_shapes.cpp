#include "parametric_shapes.hpp"
#include "core/Log.h"

#include <glm/glm.hpp>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

bonobo::mesh_data
parametric_shapes::createQuad(float const width, float const height,
                              unsigned int const horizontal_split_count,
                              unsigned int const vertical_split_count)
{
	unsigned int const horizontal_segment_count = horizontal_split_count + 1u;
	unsigned int const vertical_segment_count = vertical_split_count + 1u;
	unsigned int const cols = horizontal_segment_count + 1u;
	unsigned int const rows = vertical_segment_count + 1u;
	unsigned int const vertices_nb = rows * cols;
	
	
	auto vertices  = std::vector <glm::vec3> (vertices_nb);
	auto texcoords = std::vector <glm::vec3> (vertices_nb);
	size_t index = 0;
	
	for (unsigned int i = 0u; i < rows; ++i) {
		float const h = i*(height/vertical_segment_count);
		
		for (unsigned int j = 0u; j < cols; ++j) {
			float const w = j*(width/horizontal_segment_count);
			vertices[index] = glm::vec3(w,0.0f,h);
			
			float const u = static_cast<float>(j) / static_cast<float>(cols-1);
			float const v = static_cast<float>(i)/ static_cast<float>(rows-1);
			texcoords[index] = glm::vec3(u, v, 0.0f);
			++index;
		}
	}
	
	auto index_sets = std::vector <glm::uvec3> (2u * horizontal_segment_count * vertical_segment_count);
	index = 0;
	
	for (unsigned int i = 0u; i < rows - 1u; ++i) {
		
		for (unsigned int j = 0u; j < cols - 1u; ++j) {
			index_sets[index] = glm::uvec3(cols * i + j, cols * (i + 1u) + (j + 1u), cols * i + (j + 1u));
			
			++index;
			
			
			index_sets[index] = glm::uvec3(cols * i + j, cols * (i + 1u) + j, cols * (i + 1u) + (j + 1u));
			
			++index;
		}
		
		
	}

	bonobo::mesh_data data;

	//
	// NOTE:
	//
	// Only the values preceeded by a `\todo` tag should be changed, the
	// other ones are correct!
	//

	// Create a Vertex Array Object: it will remember where we stored the
	// data on the GPU, and  which part corresponds to the vertices, which
	// one for the normals, etc.
	//
	// The following function will create new Vertex Arrays, and pass their
	// name in the given array (second argument). Since we only need one,
	// pass a pointer to `data.vao`.
	glGenVertexArrays(1, &data.vao);

	// To be able to store information, the Vertex Array has to be bound
	// first.
	glBindVertexArray(data.vao);

	// To store the data, we need to allocate buffers on the GPU. Let's
	// allocate a first one for the vertices.
	//
	// The following function's syntax is similar to `glGenVertexArray()`:
	// it will create multiple OpenGL objects, in this case buffers, and
	// return their names in an array. Have the buffer's name stored into
	// `data.bo`.
	glGenBuffers(1, &data.bo);

	// Similar to the Vertex Array, we need to bind it first before storing
	// anything in it. The data stored in it can be interpreted in
	// different ways. Here, we will say that it is just a simple 1D-array
	// and therefore bind the buffer to the corresponding target.
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);

	auto const vertices_bytes = vertices.size() * sizeof(glm::vec3);
	auto const texcoords_bytes = texcoords.size() * sizeof(glm::vec3);

	// Allocate space for both arrays.
	glBufferData(GL_ARRAY_BUFFER, vertices_bytes + texcoords_bytes,
	             nullptr, GL_STATIC_DRAW);

	// Copy positions into the first section.
	glBufferSubData(GL_ARRAY_BUFFER, 0, vertices_bytes, vertices.data());

	// Copy UVs immediately after the positions.
	glBufferSubData(GL_ARRAY_BUFFER, vertices_bytes,
	                texcoords_bytes, texcoords.data());

	// Vertices have been just stored into a buffer, but we still need to
	// tell Vertex Array where to find them, and how to interpret the data
	// within that buffer.
	//
	// You will see shaders in more detail in lab 3, but for now they are
	// just pieces of code running on the GPU and responsible for moving
	// all the vertices to clip space, and assigning a colour to each pixel
	// covered by geometry.
	// Those shaders have inputs, some of them are the data we just stored
	// in a buffer object. We need to tell the Vertex Array which inputs
	// are enabled, and this is done by the following line of code, which
	// enables the input for vertices:
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));

	// Once an input is enabled, we need to explain where the data comes
	// from, and how it interpret it. When calling the following function,
	// the Vertex Array will automatically use the current buffer bound to
	// GL_ARRAY_BUFFER as its source for the data. How to interpret it is
	// specified below:
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices),
	                      /*! \todo how many components do our vertices have? */3,
	                      /* what is the type of each component? */GL_FLOAT,
	                      /* should it automatically normalise the values stored */GL_FALSE,
	                      /* once all components of a vertex have been read, how far away (in bytes) is the next vertex? */0,
	                      /* how far away (in bytes) from the start of the buffer is the first vertex? */reinterpret_cast<GLvoid const*>(0x0));

	// UVs contain three floats per vertex and start after the positions.
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords),
	                      3, GL_FLOAT, GL_FALSE, 0,
	                      reinterpret_cast<GLvoid const*>(vertices_bytes));

	// Now, let's allocate a second one for the indices.
	//
	// Have the buffer's name stored into `data.ibo`.
	glGenBuffers(1, /*! \todo fill me */&data.ibo);

	// We still want a 1D-array, but this time it should be a 1D-array of
	// elements, aka. indices!
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, /*! \todo bind the previously generated Buffer */data.ibo);

	glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_sets.size()*sizeof(glm::uvec3),
	             /* where is the data stored on the CPU? */index_sets.data(),
	             /* inform OpenGL that the data is modified once, but used often */GL_STATIC_DRAW);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);

	// All the data has been recorded, we can unbind them.
	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}

bonobo::mesh_data
parametric_shapes::createSphere(float const radius,
                                unsigned int const longitude_split_count,
                                unsigned int const latitude_split_count)
{
	
	unsigned int const longitude_segment_count = longitude_split_count + 1u;
	unsigned int const latitude_segment_count = latitude_split_count + 1u;
	unsigned int rows = latitude_segment_count + 1u;
	unsigned int cols = longitude_segment_count + 1u;
	
	unsigned int vertices_nb = rows * cols;
	
	auto vertices  = std::vector <glm::vec3> (vertices_nb);
	auto normals = std::vector <glm::vec3> (vertices_nb);
	auto binormals = std::vector <glm::vec3> (vertices_nb);
	auto texcoords = std::vector <glm::vec3> (vertices_nb);
	auto tangents  = std::vector<glm::vec3> (vertices_nb);
	float const d_theta = glm::two_pi<float>() / static_cast<float>(longitude_segment_count);
	float const d_phi = glm::pi<float>() / static_cast<float>(latitude_segment_count);
	
	
	
	size_t index = 0;
	float theta = 0.0f;
	float phi = 0.0f;
	
	
	for (unsigned int i = 0u; i < rows; ++i) {
		
		float rho = radius * glm::sin(phi);
		float height =  - radius * glm::cos(phi);
		//atp we have fixed some angle (of elevation) phi, then we calculate the radius rho of the smaller circle which goes around
		//the height is the y-cor which is a cos fucntion with negative cs we start at minus r
		
		
		for (unsigned int j = 0u; j < cols; ++j) {
			float x_pos = rho * glm::sin(theta);
			float z_pos = rho * glm::cos(theta);
			
			vertices[index] = glm::vec3(x_pos, height, z_pos);
			
			normals[index] = glm::normalize(glm::vec3(x_pos, height, z_pos));
			
			tangents[index] = glm::vec3(glm::cos(theta), 0.0f, -glm::sin(theta));
			
			binormals[index] = glm::normalize(glm::cross(normals[index], tangents[index]));
			
			
			//tangents[index] = glm::vec3(glm::cos(theta), 0.0f,  -glm::sin(theta));
			
			
			float const u = static_cast<float>(j) / static_cast<float>(cols-1);
			float const v = static_cast<float>(i)/ static_cast<float>(rows-1);
			
			texcoords[index] = glm::vec3(u,v,0.0f);
			++ index;
			theta += d_theta;
			
			
		}
		phi += d_phi;
		theta = 0.0f;
		
	}
	
	index = 0u;
	
	auto index_sets = std::vector <glm::uvec3> (2u * longitude_segment_count * latitude_segment_count);
	
	for (unsigned int i = 0u; i < rows - 1u; ++i) {
		
		for (unsigned int j = 0u; j < cols - 1u; ++j) {
			index_sets[index] = glm::uvec3(cols * (i + 0u) + (j+0u), cols * (i +0u) + (j + 1u), cols * (i+1u) + (j + 1u));
			
			++index;
			
			
			index_sets[index] = glm::uvec3(cols * (i + 0u) + (j+0u), cols * (i +1u) + (j + 1u), cols * (i+1u) + (j + 0u));
			
			++index;
		}
		
		
	}
	
	
	
	
	
	bonobo::mesh_data data;
	
	
	
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);
	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
												+normals_size
												+texcoords_size
												+tangents_size
												+binormals_size
												);
	
	
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
	

	
	//! \todo Implement this function

	
	
}

bonobo::mesh_data
parametric_shapes::createTorus(float const major_radius,
                               float const minor_radius,
                               unsigned int const major_split_count,
                               unsigned int const minor_split_count)
{
	if (major_radius <= minor_radius || minor_radius <= 0.0f ||
	    major_split_count < 2u || minor_split_count < 2u) {
		LogError("A torus needs major_radius > minor_radius > 0 and at least 3 segments per circle");
		return bonobo::mesh_data();
	}

	unsigned int const major_segments = major_split_count + 1u;
	unsigned int const minor_segments = minor_split_count + 1u;
	unsigned int const rows = major_segments + 1u;
	unsigned int const cols = minor_segments + 1u;

	auto vertices = std::vector<glm::vec3>(rows * cols);
	auto normals = std::vector<glm::vec3>(rows * cols);
	auto texcoords = std::vector<glm::vec3>(rows * cols);
	for (unsigned int i = 0u; i < rows; ++i) {
		float const u = static_cast<float>(i) / static_cast<float>(major_segments);
		float const theta = glm::two_pi<float>() * u;
		for (unsigned int j = 0u; j < cols; ++j) {
			float const v = static_cast<float>(j) / static_cast<float>(minor_segments);
			float const phi = glm::two_pi<float>() * v;
			float const distance = major_radius + minor_radius * std::cos(phi);
			auto const index = i * cols + j;

			vertices[index] = glm::vec3(distance * std::cos(theta),
			                            distance * std::sin(theta),
			                            minor_radius * std::sin(phi));
			normals[index] = glm::vec3(std::cos(phi) * std::cos(theta),
			                           std::cos(phi) * std::sin(theta),
			                           std::sin(phi));
			texcoords[index] = glm::vec3(u, v, 0.0f);
		}
	}

	auto triangles = std::vector<glm::uvec3>();
	triangles.reserve(2u * major_segments * minor_segments);
	for (unsigned int i = 0u; i < major_segments; ++i) {
		for (unsigned int j = 0u; j < minor_segments; ++j) {
			unsigned int const a = i * cols + j;
			unsigned int const b = (i + 1u) * cols + j;
			unsigned int const c = b + 1u;
			unsigned int const d = a + 1u;
			triangles.emplace_back(a, b, c);
			triangles.emplace_back(a, c, d);
		}
	}

	bonobo::mesh_data data;
	data.name = "Torus";
	data.vertices_nb = static_cast<GLsizei>(vertices.size());
	data.indices_nb = static_cast<GLsizei>(triangles.size() * 3u);
	glGenVertexArrays(1, &data.vao);
	glBindVertexArray(data.vao);

	auto const array_bytes = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	glGenBuffers(1, &data.bo);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, 3 * array_bytes, nullptr, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, array_bytes, vertices.data());
	glBufferSubData(GL_ARRAY_BUFFER, array_bytes, array_bytes, normals.data());
	glBufferSubData(GL_ARRAY_BUFFER, 2 * array_bytes, array_bytes, texcoords.data());

	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::vertices),
	                      3, GL_FLOAT, GL_FALSE, 0, nullptr);
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::normals),
	                      3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void const*>(array_bytes));
	glEnableVertexAttribArray(static_cast<GLuint>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<GLuint>(bonobo::shader_bindings::texcoords),
	                      3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void const*>(2 * array_bytes));

	glGenBuffers(1, &data.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	             static_cast<GLsizeiptr>(triangles.size() * sizeof(glm::uvec3)),
	             triangles.data(), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ARRAY_BUFFER, 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);
	return data;
}


bonobo::mesh_data
parametric_shapes::createCircleRing(float const radius,
                                    float const spread_length,
                                    unsigned int const circle_split_count,
                                    unsigned int const spread_split_count)
{
	auto const circle_slice_edges_count = circle_split_count + 1u;
	auto const spread_slice_edges_count = spread_split_count + 1u;
	auto const circle_slice_vertices_count = circle_slice_edges_count + 1u;
	auto const spread_slice_vertices_count = spread_slice_edges_count + 1u;
	auto const vertices_nb = circle_slice_vertices_count * spread_slice_vertices_count;

	auto vertices  = std::vector<glm::vec3>(vertices_nb);
	auto normals   = std::vector<glm::vec3>(vertices_nb);
	auto texcoords = std::vector<glm::vec3>(vertices_nb);
	auto tangents  = std::vector<glm::vec3>(vertices_nb);
	auto binormals = std::vector<glm::vec3>(vertices_nb);

	float const spread_start = radius - 0.5f * spread_length;
	float const d_theta = glm::two_pi<float>() / (static_cast<float>(circle_slice_edges_count));
	float const d_spread = spread_length / (static_cast<float>(spread_slice_edges_count));

	// generate vertices iteratively
	size_t index = 0u;
	float theta = 0.0f;
	for (unsigned int i = 0u; i < circle_slice_vertices_count; ++i) {
		float const cos_theta = std::cos(theta);
		float const sin_theta = std::sin(theta);

		float distance_to_centre = spread_start;
		for (unsigned int j = 0u; j < spread_slice_vertices_count; ++j) {
			// vertex
			vertices[index] = glm::vec3(distance_to_centre * cos_theta,
			                            distance_to_centre * sin_theta,
			                            0.0f);

			// texture coordinates
			texcoords[index] = glm::vec3(static_cast<float>(j) / (static_cast<float>(spread_slice_vertices_count)),
			                             static_cast<float>(i) / (static_cast<float>(circle_slice_vertices_count)),
			                             0.0f);

			// tangent
			auto const t = glm::vec3(cos_theta, sin_theta, 0.0f);
			tangents[index] = t;

			// binormal
			auto const b = glm::vec3(-sin_theta, cos_theta, 0.0f);
			binormals[index] = b;

			// normal
			auto const n = glm::cross(t, b);
			normals[index] = n;

			distance_to_centre += d_spread;
			++index;
		}

		theta += d_theta;
	}

	// create index array
	auto index_sets = std::vector<glm::uvec3>(2u * circle_slice_edges_count * spread_slice_edges_count);

	// generate indices iteratively
	index = 0u;
	for (unsigned int i = 0u; i < circle_slice_edges_count; ++i)
	{
		for (unsigned int j = 0u; j < spread_slice_edges_count; ++j)
		{
			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
			                               spread_slice_vertices_count * (i + 0u) + (j + 1u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 1u));
			++index;

			index_sets[index] = glm::uvec3(spread_slice_vertices_count * (i + 0u) + (j + 0u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 1u),
			                               spread_slice_vertices_count * (i + 1u) + (j + 0u));
			++index;
		}
	}

	bonobo::mesh_data data;
	glGenVertexArrays(1, &data.vao);
	assert(data.vao != 0u);
	glBindVertexArray(data.vao);

	auto const vertices_offset = 0u;
	auto const vertices_size = static_cast<GLsizeiptr>(vertices.size() * sizeof(glm::vec3));
	auto const normals_offset = vertices_size;
	auto const normals_size = static_cast<GLsizeiptr>(normals.size() * sizeof(glm::vec3));
	auto const texcoords_offset = normals_offset + normals_size;
	auto const texcoords_size = static_cast<GLsizeiptr>(texcoords.size() * sizeof(glm::vec3));
	auto const tangents_offset = texcoords_offset + texcoords_size;
	auto const tangents_size = static_cast<GLsizeiptr>(tangents.size() * sizeof(glm::vec3));
	auto const binormals_offset = tangents_offset + tangents_size;
	auto const binormals_size = static_cast<GLsizeiptr>(binormals.size() * sizeof(glm::vec3));
	auto const bo_size = static_cast<GLsizeiptr>(vertices_size
	                                            +normals_size
	                                            +texcoords_size
	                                            +tangents_size
	                                            +binormals_size
	                                            );
	glGenBuffers(1, &data.bo);
	assert(data.bo != 0u);
	glBindBuffer(GL_ARRAY_BUFFER, data.bo);
	glBufferData(GL_ARRAY_BUFFER, bo_size, nullptr, GL_STATIC_DRAW);

	glBufferSubData(GL_ARRAY_BUFFER, vertices_offset, vertices_size, static_cast<GLvoid const*>(vertices.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::vertices));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::vertices), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(0x0));

	glBufferSubData(GL_ARRAY_BUFFER, normals_offset, normals_size, static_cast<GLvoid const*>(normals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::normals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::normals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(normals_offset));

	glBufferSubData(GL_ARRAY_BUFFER, texcoords_offset, texcoords_size, static_cast<GLvoid const*>(texcoords.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::texcoords));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::texcoords), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(texcoords_offset));

	glBufferSubData(GL_ARRAY_BUFFER, tangents_offset, tangents_size, static_cast<GLvoid const*>(tangents.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::tangents));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::tangents), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(tangents_offset));

	glBufferSubData(GL_ARRAY_BUFFER, binormals_offset, binormals_size, static_cast<GLvoid const*>(binormals.data()));
	glEnableVertexAttribArray(static_cast<unsigned int>(bonobo::shader_bindings::binormals));
	glVertexAttribPointer(static_cast<unsigned int>(bonobo::shader_bindings::binormals), 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<GLvoid const*>(binormals_offset));

	glBindBuffer(GL_ARRAY_BUFFER, 0u);

	data.indices_nb = static_cast<GLsizei>(index_sets.size() * 3u);
	glGenBuffers(1, &data.ibo);
	assert(data.ibo != 0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(index_sets.size() * sizeof(glm::uvec3)), reinterpret_cast<GLvoid const*>(index_sets.data()), GL_STATIC_DRAW);

	glBindVertexArray(0u);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0u);

	return data;
}
