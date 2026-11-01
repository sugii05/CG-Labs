#include "assignment5.hpp"
#include "parametric_shapes.hpp"

#include "config.hpp"
#include "core/Bonobo.h"
#include "core/FPSCamera.h"
#include "core/helpers.hpp"
#include "core/node.hpp"
#include "core/ShaderProgramManager.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <tinyfiledialogs.h>

#include <array>
#include <chrono>
#include <clocale>
#include <cstddef>
#include <stdexcept>
#include <cstdlib>

bonobo::mesh_data createCrosshair()
{
	std::array<glm::vec3, 4> const vertices = {{
		glm::vec3(-0.03f,  0.00f, 0.0f),
		glm::vec3( 0.03f,  0.00f, 0.0f),
		glm::vec3( 0.00f, -0.03f, 0.0f),
		glm::vec3( 0.00f,  0.03f, 0.0f)
	}};

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
}

edaf80::Assignment5::Assignment5(WindowManager& windowManager) :
	mCamera(0.5f * glm::half_pi<float>(),
	        static_cast<float>(config::resolution_x) / static_cast<float>(config::resolution_y),
	        0.01f, 1000.0f),
	inputHandler(), mWindowManager(windowManager), window(nullptr)
{
	WindowManager::WindowDatum window_datum{ inputHandler, mCamera, config::resolution_x, config::resolution_y, 0, 0, 0, 0};

	window = mWindowManager.CreateGLFWWindow("Neon Run", window_datum, config::msaa_rate);
	if (window == nullptr) {
		throw std::runtime_error("Failed to get a window: aborting!");
	}

	bonobo::init();
}

edaf80::Assignment5::~Assignment5()
{
	bonobo::deinit();
}

void
edaf80::Assignment5::run()
{
	mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 8.0f));
	mCamera.mWorld.LookAt(glm::vec3(0.0f));

	ShaderProgramManager program_manager;
	GLuint ring_shader = 0u;
	program_manager.CreateAndRegisterProgram("Ring",
	                                         { { ShaderType::vertex, "EDAF80/neon_ring.vert" },
	                                           { ShaderType::fragment, "EDAF80/neon_ring.frag" } },
	                                         ring_shader);
	if (ring_shader == 0u) {
		LogError("Failed to load ring shader");
		return;
	}

	GLuint crosshair_shader = 0u;
	program_manager.CreateAndRegisterProgram("Crosshair",
	                                         { { ShaderType::vertex, "common/fallback.vert" },
	                                           { ShaderType::fragment, "common/fallback.frag" } },
	                                         crosshair_shader);
	if (crosshair_shader == 0u) {
		LogError("Failed to load crosshair shader");
		return;
	}

	float const ring_radius = 2.0f;
	float const tube_radius = 0.25f;
	float const opening_radius = ring_radius - tube_radius;
	auto const ring_mesh = parametric_shapes::createTorus(ring_radius, tube_radius, 63u, 15u);
	if (ring_mesh.vao == 0u) {
		LogError("Failed to create torus geometry");
		return;
	}

	auto const crosshair_mesh = createCrosshair();
	if (crosshair_mesh.vao == 0u) {
		LogError("Failed to create crosshair geometry");
		glDeleteBuffers(1, &ring_mesh.ibo);
		glDeleteBuffers(1, &ring_mesh.bo);
		glDeleteVertexArrays(1, &ring_mesh.vao);
		return;
	}
	Node crosshair;
	crosshair.set_geometry(crosshair_mesh);
	crosshair.set_program(&crosshair_shader);
	crosshair.get_transform().SetTranslate(glm::vec3(0.0f, 0.0f, -2.5f));

	std::array<Node, 8> rings;
	std::array<glm::vec2, 8> const ring_offsets = {{
		glm::vec2( 0.0f,  0.0f), glm::vec2( 2.0f,  0.0f),
		glm::vec2( 2.0f,  2.0f), glm::vec2( 0.0f,  2.0f),
		glm::vec2(-2.0f,  2.0f), glm::vec2(-2.0f,  0.0f),
		glm::vec2(-2.0f, -2.0f), glm::vec2( 0.0f, -2.0f)
	}};

	std::array<float , 8> ring_angular_speeds {};

	float const ring_spacing = 6.0f;
	float const ring_start_z = -12.0f;
	float const ring_end_z = 10.0f;
	float const loop_length = ring_spacing * static_cast<float>(rings.size());
	float ring_speed = 4.0f;
	float const steering_speed = 4.0f;

	bool pause_motion = false;
	bool game_over = false;
	bool restart_requested = true;
	int score = 0;
	int missed_rings = 0;
	char const* last_result = "Steer through the ring openings.";

	for (std::size_t i = 0u; i < rings.size(); ++i) {
		auto& ring = rings[i];
		int const random_number = std::rand() % 9;
		float spin_speed = 0.4f + 0.1f * random_number;
		ring_angular_speeds[i] = spin_speed;
		ring.set_geometry(ring_mesh);
		ring.set_program(&ring_shader, [this](GLuint program) {
			auto const light_position = glm::vec3(-3.0f, 4.0f, 6.0f);
			glUniform3fv(glGetUniformLocation(program, "light_position"),
			             1, glm::value_ptr(light_position));
			auto const camera_position = mCamera.mWorld.GetTranslation();
			glUniform3fv(glGetUniformLocation(program, "camera_position"),
			             1, glm::value_ptr(camera_position));
		});
	}

	glClearDepthf(1.0f);
	glClearColor(0.015f, 0.02f, 0.05f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	auto lastTime = std::chrono::steady_clock::now();

	bool show_logs = false;
	auto polygon_mode = bonobo::polygon_mode_t::fill;
	bool show_gui = true;
	bool shader_reload_failed = false;
	bool show_basis = false;
	float basis_thickness_scale = 1.0f;
	float basis_length_scale = 1.0f;

	while (!glfwWindowShouldClose(window)) {
		auto const nowTime = std::chrono::steady_clock::now();
		auto const deltaTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(nowTime - lastTime);
		lastTime = nowTime;
		float const dt = glm::min(std::chrono::duration<float>(deltaTimeUs).count(), 0.1f);

		auto& io = ImGui::GetIO();
		inputHandler.SetUICapture(io.WantCaptureMouse, io.WantCaptureKeyboard);

		glfwPollEvents();
		inputHandler.Advance();

		if (inputHandler.GetKeycodeState(GLFW_KEY_R) & JUST_PRESSED) {
			shader_reload_failed = !program_manager.ReloadAllPrograms();
			if (shader_reload_failed)
				tinyfd_notifyPopup("Shader Program Reload Error",
				                   "An error occurred while reloading shader programs; see the logs for details.\n"
				                   "Rendering is suspended until the issue is solved. Once fixed, just reload the shaders again.",
				                   "error");
		}
		if (inputHandler.GetKeycodeState(GLFW_KEY_F3) & JUST_RELEASED)
			show_logs = !show_logs;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F2) & JUST_RELEASED)
			show_gui = !show_gui;
		if (inputHandler.GetKeycodeState(GLFW_KEY_F11) & JUST_RELEASED)
			mWindowManager.ToggleFullscreenStatusForWindow(window);

		int framebuffer_width, framebuffer_height;
		glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
		glViewport(0, 0, framebuffer_width, framebuffer_height);

		if (restart_requested) {
			game_over = false;
			mCamera.mWorld.SetTranslate(glm::vec3(0.0f, 0.0f, 8.0f));
			score = 0;
			missed_rings = 0;
			last_result = "Steer through the ring openings.";
			pause_motion = false;
			for (std::size_t i = 0u; i < rings.size(); ++i) {
				float const z = ring_start_z - ring_spacing * static_cast<float>(i);
				rings[i].get_transform().SetTranslate(glm::vec3(ring_offsets[i], z));
				rings[i].get_transform().SetRotateZ(0.0f);
			}
			restart_requested = false;
		}

		if (!pause_motion && !shader_reload_failed && !game_over) {
			auto const previous_camera_position = mCamera.mWorld.GetTranslation();
			auto camera_position = previous_camera_position;
			glm::vec3 direction(0.0f);
			if (!inputHandler.IsKeyboardCapturedByUI()) {
				if (inputHandler.GetKeycodeState(GLFW_KEY_A) & PRESSED) direction.x -= 1.0f;
				if (inputHandler.GetKeycodeState(GLFW_KEY_D) & PRESSED) direction.x += 1.0f;
				if (inputHandler.GetKeycodeState(GLFW_KEY_W) & PRESSED) direction.y += 1.0f;
				if (inputHandler.GetKeycodeState(GLFW_KEY_S) & PRESSED) direction.y -= 1.0f;
			}
			if (glm::length(direction) > 0.0f) {
				camera_position += glm::normalize(direction) * steering_speed * dt;
			}
			camera_position.x = glm::clamp(camera_position.x, -4.0f, 4.0f);
			camera_position.y = glm::clamp(camera_position.y, -4.0f, 4.0f);
			mCamera.mWorld.SetTranslate(camera_position);

			for (std::size_t i = 0u; i < rings.size(); i++ ){
				auto& ring = rings[i];
				auto position = ring.get_transform().GetTranslation();
				float const previous_z = position.z;
				position.z += ring_speed * dt;
				ring.get_transform().RotateZ(ring_angular_speeds[i] * dt);

				if (previous_z < camera_position.z && position.z >= camera_position.z) {
					float const crossing_fraction = (camera_position.z - previous_z) / (position.z - previous_z);
					auto const crossing_position = previous_camera_position
					    + crossing_fraction * (camera_position - previous_camera_position);
					float const distance = glm::length(glm::vec2(crossing_position - position));
					if (distance < opening_radius) {
						++score;
						last_result = "Through the opening! +1 point";
					} else {
						if (distance <= ring_radius + tube_radius) {
								game_over = true;
								last_result = "Game over! You hit a ring. Click Restart.";
								ring.get_transform().SetTranslate(position);
								break;
							}

							last_result = "Missed. Aim for the next opening.";
					}
				}

				while (position.z >= ring_end_z) {
					position.z -= loop_length;
				}
				ring.get_transform().SetTranslate(position);
			}
		}

		mWindowManager.NewImGuiFrame();

		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		if (!shader_reload_failed) {
			bonobo::changePolygonMode(polygon_mode);
			for (auto const& ring : rings) {
				ring.render(mCamera.GetWorldToClipMatrix());
			}
			glDisable(GL_DEPTH_TEST);
			crosshair.render(mCamera.GetWorldToClipMatrix(), mCamera.GetViewToWorldMatrix());
			glEnable(GL_DEPTH_TEST);
		}

		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		bool const opened = ImGui::Begin("Neon Run", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		if (opened) {
			ImGui::TextUnformatted("W/A/S/D: up / left / down / right");
			ImGui::TextUnformatted("Keep the crosshair inside each opening.");
			ImGui::Text("Score: %d | Missed: %d", score, missed_rings);
			ImGui::TextUnformatted(last_result);
			if (ImGui::Button("Restart")) restart_requested = true;
			ImGui::Checkbox("Pause", &pause_motion);
			ImGui::SliderFloat("Speed (units/s)", &ring_speed, 1.0f, 10.0f);
			bonobo::uiSelectPolygonMode("Polygon mode", polygon_mode);
			ImGui::Checkbox("Show basis", &show_basis);
			ImGui::SliderFloat("Basis thickness scale", &basis_thickness_scale, 0.0f, 100.0f);
			ImGui::SliderFloat("Basis length scale", &basis_length_scale, 0.0f, 100.0f);
		}
		ImGui::End();

		if (show_basis)
			bonobo::renderBasis(basis_thickness_scale, basis_length_scale, mCamera.GetWorldToClipMatrix());
		if (show_logs)
			Log::View::Render();
		mWindowManager.RenderImGuiFrame(show_gui);

		glfwSwapBuffers(window);
	}

	glDeleteBuffers(1, &crosshair_mesh.bo);
	glDeleteVertexArrays(1, &crosshair_mesh.vao);
	glDeleteBuffers(1, &ring_mesh.ibo);
	glDeleteBuffers(1, &ring_mesh.bo);
	glDeleteVertexArrays(1, &ring_mesh.vao);
}

int main()
{
	std::setlocale(LC_ALL, "");

	Bonobo framework;

	try {
		edaf80::Assignment5 assignment5(framework.GetWindowManager());
		assignment5.run();
	} catch (std::runtime_error const& e) {
		LogError(e.what());
	}
}
