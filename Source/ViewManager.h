///////////////////////////////////////////////////////////////////////////////
// viewmanager.h
// ============
// manage the viewing of 3D objects within the viewport
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ShaderManager.h"
#include "camera.h"

// GLFW library
#include "GLFW/glfw3.h" 

class ViewManager
{
public:
	// constructor
	ViewManager(
		ShaderManager* pShaderManager);
	// destructor
	~ViewManager();

	// mouse position callback for mouse interaction with the 3D scene
	static void Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos);

//*******************************************************************************************************************************************************************************
	// mouse scroll callback for mouse scroll interaction with the 3D scene - makes movements fast/slow
	static void Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset);

	// Methods for setting projection types
	void SetPerspective(); // Default Perspective
	void SetOrthographic(); // Set Orthographic Projection
	glm::mat4 GetProjectionMatrix() const; // Gets projection matrix

//*******************************************************************************************************************************************************************************
private:
	// pointer to shader manager object
	ShaderManager* m_pShaderManager;
	// active OpenGL display window
	GLFWwindow* m_pWindow;

	// process keyboard events for interaction with the 3D scene
	void ProcessKeyboardEvents();

//*******************************************************************************************************************************************************************************
	glm::mat4 projection; // Projection matrix
	float aspectRatio; // window aspect ratio for 3D->2D conversion

//*******************************************************************************************************************************************************************************
public:
	// create the initial OpenGL display window
	GLFWwindow* CreateDisplayWindow(const char* windowTitle);
	
	// prepare the conversion from 3D object display to 2D scene display
	void PrepareSceneView();
};
//*******************************************************************************************************************************************************************************
//*******************************************************************************************************************************************************************************