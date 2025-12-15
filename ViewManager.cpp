///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = glm::vec3(0.0f, 5.0f, 12.0f);
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// this callback is used to receive mouse wheel scrolling events
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	if (gFirstMouse)
	{
		gLastX = static_cast<float>(xMousePos);
		gLastY = static_cast<float>(yMousePos);
		gFirstMouse = false;
	}

	float xOffset = static_cast<float>(xMousePos) - gLastX;
	float yOffset = gLastY - static_cast<float>(yMousePos); // inverted y

	gLastX = static_cast<float>(xMousePos);
	gLastY = static_cast<float>(yMousePos);

	// Use Camera's built-in mouse movement processor
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse zooms in/out within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	if (g_pCamera)
	{
		// yOffset > 0: scroll up, increase speed
		// yOffset < 0: scroll down, decrease speed
		g_pCamera->ProcessMouseScroll(static_cast<float>(yOffset));
	}
	std::cout << "SCROLL yOffset = " << yOffset << std::endl;
}





/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}
	// move the camera forward.
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS) {
		// zoom into 3D scene
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}

	//move the camera backward.
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS) {
		// zoom out of 3D scene
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	//move the camera left.
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS) {
		// pan the camera out of the 3D Scene.
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}

	// move the camera right,
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS) {
		// pan the camera out of the 3D scene.
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}


	//move the camera upward.
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) {
		// pan the camera upward
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}

	//move the camera down.
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS) {
		// pan the camera down.
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// Switch to perspective view
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) {
		bOrthographicProjection = false;
	}

	// Switch to orthographic view
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) {
		bOrthographicProjection = true;
	}

}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the event queue
	ProcessKeyboardEvents();

	// define the current projection matrix based on the current mode
	if (!bOrthographicProjection)
	{
		// Perspective projection (3D)
		view = g_pCamera->GetViewMatrix();

		projection = glm::perspective(
			glm::radians(g_pCamera->Zoom),
			(GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT,
			0.1f,
			100.0f
		);
	}
	else
	{
		// Orthographic projection (front view 2D)
		float scale = 10.0f; // adjust to fit your scene
		float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

		projection = glm::ortho(
			-scale * aspectRatio, scale * aspectRatio, // left/right
			-scale, scale,                             // bottom/top
			0.1f, 100.0f                               // near/far
		);

		// Front-view camera setup
		g_pCamera->Position = glm::vec3(0.0f, 0.0f, 10.0f);  // in front of origin
		g_pCamera->Front = glm::vec3(0.0f, 0.0f, -1.0f); // look at origin
		g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);  // Y is up

		// Compute view matrix explicitly for orthographic front view
		view = glm::lookAt(
			g_pCamera->Position,
			g_pCamera->Position + g_pCamera->Front,
			g_pCamera->Up
		);
	}

	// if the shader manager object is valid
	if (m_pShaderManager != nullptr)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);

		// set the projection matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);

		// set the camera position into the shader
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
	}
}

