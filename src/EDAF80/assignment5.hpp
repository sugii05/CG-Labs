#pragma once

#include "core/InputHandler.h"
#include "core/FPSCamera.h"
#include "core/WindowManager.hpp"

class Window;

namespace edaf80
{
	class Assignment5 {
	public:
		Assignment5(WindowManager& windowManager);

		~Assignment5();

		void run();

	private:
		FPSCameraf     mCamera;
		InputHandler   inputHandler;
		WindowManager& mWindowManager;
		GLFWwindow*    window;
	};
}
