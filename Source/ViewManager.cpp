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
	const int WINDOW_WIDTH = 1400;
	const int WINDOW_HEIGHT = 1200;
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

//*******************************************************************************************************************************************************************************
//ViewManager() - Constructor for the class
ViewManager::ViewManager(ShaderManager* pShaderManager) : m_pShaderManager(pShaderManager), aspectRatio(1000.0f / 800.0f) { // Initialize shader manager and aspect ratio
	m_pWindow = NULL; // Initialize window pointer to NULL
	g_pCamera = new Camera(); // Allocate memory for camera

	// Custom Default camera view parameters
	g_pCamera->Position = glm::vec3(3.0f, 12.0f, 15.0f); // Set camera position
	g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f); // Set camera front vector
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f); // Set camera up vector
	g_pCamera->Zoom = 80; // Set camera zoom level
	SetPerspective(); // Default persasspective projection


	// Default camera view parameters
	//g_pCamera->Position = glm::vec3(0.5f, 5.5f, 10.0f); // Set camera position
	//g_pCamera->Front = glm::vec3(0.0f, -0.5f, -2.0f); // Set camera front vector
	//g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f); // Set camera up vector
	//g_pCamera->Zoom = 80; // Set camera zoom level
	//SetPerspective(); // Default perspective projection
}

//*******************************************************************************************************************************************************************************
//ViewManager() - Destructor for the class
ViewManager::~ViewManager() {
	// Free allocated memory
	m_pShaderManager = NULL; // Set shader manager pointer to NULL
	m_pWindow = NULL; // Set window pointer to NULL
	if (g_pCamera != NULL) { // If camera is not NULL
		delete g_pCamera; // Delete camera object
		g_pCamera = NULL; // Set camera pointer to NULL
	}
}

//*******************************************************************************************************************************************************************************
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle) {
	GLFWwindow* window = nullptr;

	// Try to create OpenGL window
	window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, windowTitle, NULL, NULL); // Create window
	if (window == NULL) { // If window creation fails
		std::cout << "Failed to create GLFW window" << std::endl; // Output error message
		glfwTerminate(); // Terminate GLFW
		return NULL; // Return NULL
	}
	glfwMakeContextCurrent(window); // Make window context current

	// Set callbacks for mouse events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback); // Set mouse position callback
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback); // Set mouse scroll callback

	// Enable blending for transparent rendering
	glEnable(GL_BLEND); // Enable blending
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Set blend function

	m_pWindow = window; // Set window pointer

	return window; // Return created window
}

//*******************************************************************************************************************************************************************************
//Mouse_Position_Callback() - Called by GLFW when mouse moves
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos) {
	// If this is the first mouse event, reinitialize gLastX and gLastY
	if (gFirstMouse) {
		gLastX = xMousePos; // Record the current X position
		gLastY = yMousePos-1500; // Record the current Y position
		gFirstMouse = false; // Disable the first mouse flag
		return; // Return immediately to prevent a sudden jump
	}

	// Calculate offsets for moving 3D camera
	float xOffset = xMousePos - gLastX; // Calculate X offset
	float yOffset = gLastY - yMousePos; // Calculate Y offset

	// Update last positions
	gLastX = xMousePos; // Update last X position
	gLastY = yMousePos; // Update last Y position

	// Move 3D camera angle based on offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset); // Move camera angle
}

void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset) {
	// Control camera speed based on scroll offset
	g_pCamera->ProcessMouseScroll(yOffset); // Controls speed
}

//*******************************************************************************************************************************************************************************
//ProcessKeyboardEvents() - Process any keyboard events
void ViewManager::ProcessKeyboardEvents() {
	// Close window if escape key is pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS) { // If escape key pressed
		glfwSetWindowShouldClose(m_pWindow, true); // Close window
	}

	// If camera object is null, exit
	if (g_pCamera == nullptr) { // If camera is NULL
		return; // Exit method
	}

	// Process camera movements
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS) { // If W key pressed
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime); // Move camera: forward
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS) { // If S key pressed
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime); // Move camera: backward
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS) { // If A key pressed
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime); // Pan camera: left
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS) { // If D key pressed
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime); // Pan camera: right
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS) { // If Q key pressed
		g_pCamera->ProcessKeyboard(UP, gDeltaTime); // Move camera: up
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS) { // If E key pressed
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime); // Move camera: down
	}

	// Switch projection modes
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS) { // If P key pressed
		SetPerspective(); // Set perspective projection
		bOrthographicProjection = false; // Disable orthographic projection 
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS) { // If O key pressed
		SetOrthographic(); // Set orthographic projection
		bOrthographicProjection = true; // Enable orthographic projection 
	}
}

//*******************************************************************************************************************************************************************************
//PrepareSceneView() - Prepare 3D scene for rendering
void ViewManager::PrepareSceneView() {
	glm::mat4 view;

	// Per-frame timing
	float currentFrame = glfwGetTime(); // Get current frame time
	gDeltaTime = currentFrame - gLastFrame; // Calculate delta time
	gLastFrame = currentFrame; // Update last frame time

	// Process any keyboard events
	ProcessKeyboardEvents(); // Process keyboard events

	// Get current view matrix from camera
	view = g_pCamera->GetViewMatrix(); // Get view matrix

	// If shader manager is valid
	if (m_pShaderManager != nullptr) {
		// Set view, projection matrix and camera position in shader
		m_pShaderManager->setMat4Value(g_ViewName, view); // Set view matrix
		m_pShaderManager->setMat4Value(g_ProjectionName, projection); // Set projection matrix
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position); // Set camera position
	}
}

//*******************************************************************************************************************************************************************************
//SetPerspective() - Set projection matrix to perspective
void ViewManager::SetPerspective() {
	projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f); // Set perspective projection matrix
}

//*******************************************************************************************************************************************************************************
// SetOrthographic() - Set projection matrix to orthographic
void ViewManager::SetOrthographic() {
	float orthoScale = 10.0f; // Ortho projection size

	// Create base orthographic projection matrix
	glm::mat4 orthoProjection = glm::ortho(-aspectRatio * orthoScale, aspectRatio * orthoScale, -orthoScale, orthoScale, 0.1f, 100.0f);

	// Apply rotation to projection matrix
	float angle = glm::radians(52.5f); // Angle for rotation
	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1.0f, 0.0f, 0.0f)); // Rotate around X-axis

	// Scale to reduce width and correct stretch
	float correctionFactorX = 0.77f;  // Reduce width
	float correctionFactorY = 1.0f / sin(angle);  // Correct Y-axis
	glm::mat4 scalingMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(correctionFactorX, correctionFactorY, 1.0f)); // Apply scaling

	// Combine projection, rotation, and scaling
	projection = scalingMatrix * rotationMatrix * orthoProjection; // Final matrix

}
//Default Ortho View
	//float orthoScale = 10.0f; // Scaling to define size of ortho projection
	//projection = glm::ortho(-aspectRatio * orthoScale, aspectRatio * orthoScale, -orthoScale, orthoScale, 0.1f, 100.0f); // Set orthographic projection matrix

//*******************************************************************************************************************************************************************************
//GetProjectionMatrix() - Return current projection matrix
glm::mat4 ViewManager::GetProjectionMatrix() const {
	return projection; // Return current projection matrix
}

//*******************************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************************
