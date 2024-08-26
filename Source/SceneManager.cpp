///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}
//**************************************************************************************************************************************************
//*********************************************************************************************************************************************************************************************
//**************************************************************************************************************************************************
//DONT EDIT ABOVE








































































































//**********************************************************************************
//█▀ ▀█▀ █░█ █▀▄ █▀▀ █▄░█ ▀█▀   █▀▀ █▀▄ █ ▀█▀   ▀█ █▀█ █▄░█ █▀▀
//▄█ ░█░ █▄█ █▄▀ ██▄ █░▀█ ░█░   ██▄ █▄▀ █ ░█░   █▄ █▄█ █░▀█ ██▄
//**********************************************************************************



//**********************************************************************************
//▀█▀ █▀▀ ▀▄▀ ▀█▀ █░█ █▀█ █▀▀   █▀▀ █ █░░ █▀▀ █▀
//░█░ ██▄ █░█ ░█░ █▄█ █▀▄ ██▄   █▀░ █ █▄▄ ██▄ ▄█
//**********************************************************************************
//LoadSceneTextures() - used for loading the texturs of the scene objects
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	//▀█▀ ▄▀█ █▄▄ █░░ █▀▀
	//░█░ █▀█ █▄█ █▄▄ ██▄
	bReturn = CreateGLTexture(
		"../../Utilities/textures/metal_table.jpg",
		"metal_table");

	//█░█ ▄▀█ █▀ █▀▀
	//▀▄▀ █▀█ ▄█ ██▄
	bReturn = CreateGLTexture(
		"../../Utilities/textures/blue_vase.jpg",
		"blue_vase");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/blue_vase3.jpg",
		"blue_vase3");

	//░░█ █░█ █▀▀
	//█▄█ █▄█ █▄█
	bReturn = CreateGLTexture(
		"../../Utilities/textures/tiger_wood.jpg",
		"tiger_wood");

	//█░█░█ █▀▀ █ █▀▀ █░█ ▀█▀
	//▀▄▀▄▀ ██▄ █ █▄█ █▀█ ░█░
	bReturn = CreateGLTexture(
		"../../Utilities/textures/pink_matte.jpg",
		"pink_matte");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/pink_matte2.jpg",
		"pink_matte2");

	 //3  █▀▄ █▀
	//	  █▄▀ ▄█
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ruby4.jpg",
		"ruby4");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ruby6.jpg",
		"ruby6");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ruby8.jpg",
		"ruby8");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/ruby9.jpg",
		"ruby9");

	//▀█▀ █▀█ ▄▀█ █▀ █░█   █▀▀ ▄▀█ █▄░█
	//░█░ █▀▄ █▀█ ▄█ █▀█   █▄▄ █▀█ █░▀█
	bReturn = CreateGLTexture(
		"../../Utilities/textures/trash1.jpg",
		"trash1");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/can_skin.jpg",
		"can_skin");

	//█▀▀ ▀▄▀ ▀█▀ █▀█ ▄▀█
	//██▄ █░█ ░█░ █▀▄ █▀█
	bReturn = CreateGLTexture(
		"../../Utilities/textures/matte_rubber.jpg",
		"matte_rubber");
	bReturn = CreateGLTexture(
		"../../Utilities/textures/porcelain_vase.jpg",
		"porcelain_vase");

	BindGLTextures();
}

//**********************************************************************************
//█▀█ █▄▄ ░░█ █▀▀ █▀▀ ▀█▀   █▀▄▀█ ▄▀█ ▀█▀ █▀▀ █▀█ █ ▄▀█ █░░ █▀
//█▄█ █▄█ █▄█ ██▄ █▄▄ ░█░   █░▀░█ █▀█ ░█░ ██▄ █▀▄ █ █▀█ █▄▄ ▄█
//**********************************************************************************
//DefineObjectMaterials() - used for configuring material settings for objects within scene.
void SceneManager::DefineObjectMaterials()
{
	{	//Shiny - Highest level shiny
		OBJECT_MATERIAL shinyMaterial;
		shinyMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Ambient brightness
		shinyMaterial.ambientStrength = 0.7f;  // Ambient light strength
		shinyMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.2f);  // Diffuse Color
		shinyMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Specular Color
		shinyMaterial.shininess = 50.0f;  // Shiny Level
		shinyMaterial.tag = "shiny"; // Material Tag
		m_objectMaterials.push_back(shinyMaterial);
	}
	{	//Shinyish - High level shiny
		OBJECT_MATERIAL shinyMaterial;
		shinyMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Ambient brightness
		shinyMaterial.ambientStrength = 0.5f;  // Ambient light strength
		shinyMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.2f);  // Diffuse Color
		shinyMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Specular Color
		shinyMaterial.shininess = 25.0f;  // Shiny Level
		shinyMaterial.tag = "shinyish"; // Material Tag
		m_objectMaterials.push_back(shinyMaterial);
	}
	{	//Porcelaine - Mid level shin
		OBJECT_MATERIAL porcelainMaterial;
		porcelainMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Ambient brightness
		porcelainMaterial.ambientStrength = 0.5f;  // Ambient light strength
		porcelainMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.2f);  // Diffuse Color
		porcelainMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Specular Color
		porcelainMaterial.shininess = 16.0f;  // Shiny Level
		porcelainMaterial.tag = "porcelaine"; // Material Tag
		m_objectMaterials.push_back(porcelainMaterial);
	}
	{	//Dull - Low level shiny
		OBJECT_MATERIAL dullMaterial;
		dullMaterial.ambientColor = glm::vec3(0.3f, 0.3f, 0.3f);  // Ambient brightness
		dullMaterial.ambientStrength = 0.6f;  // Ambient light strength
		dullMaterial.diffuseColor = glm::vec3(0.8f, 0.7f, 0.2f);  // Diffuse Color
		dullMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);  // Specular Color
		dullMaterial.shininess = 1.0f;  // Shiny Level
		dullMaterial.tag = "dull"; // Material Tag
		m_objectMaterials.push_back(dullMaterial);
	}
	{	//Void material - Least Shiny
		OBJECT_MATERIAL voidMaterial;
		voidMaterial.ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);  // Ambient brightness
		voidMaterial.ambientStrength = 0.0f;  // Ambient light strength
		voidMaterial.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);  // Diffuse Color
		voidMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);  // Specular Color
		voidMaterial.shininess = 0.0f;  // Shiny Level
		voidMaterial.tag = "void"; // Material Tag
		m_objectMaterials.push_back(voidMaterial);
	}
}
//**********************************************************************************
//█░░ █ █▀▀ █░█ ▀█▀ █ █▄░█ █▀▀
//█▄▄ █ █▄█ █▀█ ░█░ █ █░▀█ █▄█
//**********************************************************************************
//SetupSceneLights() - called to add and configure light sources for the scene. 
void SceneManager::SetupSceneLights()
{
	// Enable or disable lighting based on the flag (uncomment to change to default lights).
	m_pShaderManager->setBoolValue(g_UseLightingName, true); // Mute this line to switch to default lighting.

	// Store the positions of the lights
	lightPositions[0] = glm::vec3(-100.0f, 40.0f, 50.0f); // Red Box Light - Positioned to illuminate left side
	lightPositions[1] = glm::vec3(-150.0f, 40.0f, -25.0f); // Green Box Light - Positioned to illuminate middle/left area
	lightPositions[2] = glm::vec3(100.0f, 20.0f, 10.0f); // Blue Box Light - Positioned TV-style lighting
	lightPositions[3] = glm::vec3(20.0f, 50.0f, -100.0f); // Yellow Box Light - Positioned to simulate sunlight

	// Configure lighting for Red Box (Left Object)
	m_pShaderManager->setVec3Value("lightSources[0].position", lightPositions[0]); // Set light position
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.0f, 0.0f, 0.0f); // Set ambient color
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.0f, 0.0f, 0.0f); // Set diffuse color
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.0f, 0.0f, 0.0f); // Set specular color
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 50.0f); // Set focal strength
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.4f); // Set specular intensity

	// Configure lighting for Green Box (Middle Object)
	m_pShaderManager->setVec3Value("lightSources[1].position", lightPositions[1]); // Set light position
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.0f, 0.0f, 0.1f); // Set ambient color
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.0f, 0.0f, 0.0f); // Set diffuse color
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.0f, 0.0f, 0.0f); // Set specular color
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 30.0f); // Set focal strength
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.1f); // Set specular intensity

	// Configure lighting for Blue Box (TV Light)
	m_pShaderManager->setVec3Value("lightSources[2].position", lightPositions[2]); // Set light position
	m_pShaderManager->setVec3Value("lightSources[2].ambientColor", 0.0f, 0.0f, 0.3f); // Set ambient color
	m_pShaderManager->setVec3Value("lightSources[2].diffuseColor", 0.0f, 0.0f, 0.2f); // Set diffuse color
	m_pShaderManager->setVec3Value("lightSources[2].specularColor", 0.0f, 0.0f, 2.0f); // Set specular color
	m_pShaderManager->setFloatValue("lightSources[2].focalStrength", 100.0f); // Set focal strength
	m_pShaderManager->setFloatValue("lightSources[2].specularIntensity", 1.0f); // Set specular intensity

	// Configure lighting for Yellow Box (Sunlight)
	m_pShaderManager->setVec3Value("lightSources[3].position", lightPositions[3]); // Set light position
	m_pShaderManager->setVec3Value("lightSources[3].ambientColor", 0.0f, 0.0f, 0.0f); // Set ambient color
	m_pShaderManager->setVec3Value("lightSources[3].diffuseColor", 0.0f, 0.0f, 0.0f); // Set diffuse color
	m_pShaderManager->setVec3Value("lightSources[3].specularColor", 0.0f, 0.0f, 0.0f); // Set specular color
	m_pShaderManager->setFloatValue("lightSources[3].focalStrength", 12.0f); // Set focal strength
	m_pShaderManager->setFloatValue("lightSources[3].specularIntensity", 0.2f); // Set specular intensity

	// Configure additional light source
	m_pShaderManager->setVec3Value("lightSources[4].position", glm::vec3(-30.0f, 40.0f, 30.0f)); // Set position for extra light
	m_pShaderManager->setVec3Value("lightSources[4].ambientColor", 0.0f, 0.0f, 0.0f); // Set ambient color for extra light
	m_pShaderManager->setVec3Value("lightSources[4].diffuseColor", 0.0f, 0.0f, 0.0f); // Set diffuse color for extra light
	m_pShaderManager->setVec3Value("lightSources[4].specularColor", 0.3f, 0.3f, 0.3f); // Set specular color for extra light
	m_pShaderManager->setFloatValue("lightSources[4].focalStrength", 30.0f); // Set focal strength for extra light
	m_pShaderManager->setFloatValue("lightSources[4].specularIntensity", 0.3f); // Set specular intensity for extra light

}
//**********************************************************************************
//█▀█ █▀█ █▀▀ █▀█ ▄▀█ █▀█ █▀▀   █▀ █▀▀ █▀▀ █▄░█ █▀▀
//█▀▀ █▀▄ ██▄ █▀▀ █▀█ █▀▄ ██▄   ▄█ █▄▄ ██▄ █░▀█ ██▄
//**********************************************************************************
// PrepareScene() - Prepare the scene for rendering
void SceneManager::PrepareScene()
{
	SetupSceneLights(); //Sets up the lights for scene
	DefineObjectMaterials(); //Sets up the Object Materials
	LoadSceneTextures(); //Sets up the textures

	// load shape meshes
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}
//**********************************************************************************
//█▀█ █▀▀ █▄░█ █▀▄ █▀▀ █▀█   █▀ █▀▀ █▀▀ █▄░█ █▀▀
//█▀▄ ██▄ █░▀█ █▄▀ ██▄ █▀▄   ▄█ █▄▄ ██▄ █░▀█ ██▄
//**********************************************************************************
//RenderScene() - used for rendering the 3D scene by transforming and drawing the basic 3D shapes
void SceneManager::RenderScene()
{
	// Declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;
//**************************************************************************************************************************************************
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█   █▀█   ▄▄   █▀▀ █░░ █▀█ █▀█ █▀█
//█ ░█░ ██▄ █░▀░█   █▄█   ░░   █▀░ █▄▄ █▄█ █▄█ █▀▄
	// Create floor plane
	scaleXYZ = glm::vec3(12.0f, 1.0f, 8.0f); // Scale shape
	positionXYZ = glm::vec3(2.5f, 0.0f, -12.0f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderTexture("metal_table"); // Set material
	SetShaderMaterial("dull"); // Set texture
	m_basicMeshes->DrawPlaneMesh(); // Draw Shape
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█   ▄█   ▄▄   █▀ █▀▄▀█ ▄▀█ █░░ █░░   █░█ ▄▀█ █▀ █▀▀
//█ ░█░ ██▄ █░▀░█   ░█   ░░   ▄█ █░▀░█ █▀█ █▄▄ █▄▄   ▀▄▀ █▀█ ▄█ ██▄
//**********************************************************************************
	// Create Sphere - Vase Body
	scaleXYZ = glm::vec3(2.0f, 2.0f, 2.0f); // Scale shape
	positionXYZ = glm::vec3(0.0f, 2.0f, -8.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderTexture("blue_vase"); // Set texture
	m_basicMeshes->DrawSphereMesh(); // Draw Shape

	// Create Cylinder - Vase Neck
	scaleXYZ = glm::vec3(0.7f, 3.0f, 0.7f); // Scale shape
	positionXYZ = glm::vec3(0.0f, 2.0f, -8.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderTexture("blue_vase3"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Vase Hole
	scaleXYZ = glm::vec3(0.7f, 0.2f, 0.7f); // Scale shape
	positionXYZ = glm::vec3(0.0f, 4.9f, -8.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("void"); // Set material
	SetShaderColor(0, 0, 0, 1); // Set color
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Torus 1 - Top Lip
	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f); // Scale shape
	XrotationDegrees = 90.0f;// Rotate Shape
	positionXYZ = glm::vec3(0.0f, 5.0f, -8.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderTexture("blue_vase3"); // Set texture
	m_basicMeshes->DrawTorusMesh(); // Draw Shape

	// Create Torus 2 - Bottom Edge
	scaleXYZ = glm::vec3(0.6f, 1.0f, 0.6f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(0.0f, 0.14f, -8.85f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderTexture("blue_vase3"); // Set texture
	m_basicMeshes->DrawTorusMesh(); // Draw Shape
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█   ▀█   ▄▄   █░█░█ ▄▀█ ▀█▀ █▀▀ █▀█   ░░█ █░█ █▀▀
//█ ░█░ ██▄ █░▀░█   █▄   ░░   ▀▄▀▄▀ █▀█ ░█░ ██▄ █▀▄   █▄█ █▄█ █▄█
//**********************************************************************************
	// Create Cylinder - Jug Body
	scaleXYZ = glm::vec3(2.5f, 5.0f, 2.5f); // Scale shape
	XrotationDegrees = 180.0f; // Rotate Shape
	positionXYZ = glm::vec3(-5.0f, 5.0f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("tiger_wood"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Tapered Cylinder - Slanted connector for cylinders
	scaleXYZ = glm::vec3(2.5f, 0.6f, 2.5f); // Scale shape
	XrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(-5.0f, 5.0f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderTexture("tiger_wood"); // Set texture
	m_basicMeshes->DrawTaperedCylinderMesh(); // Draw Shape

	// Create Cylinder - Top grey ring
	scaleXYZ = glm::vec3(1.9f, 1.5f, 1.9f); // Scale shape
	XrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(-5.0f, 4.3f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("matte_rubber"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Black Hole
	scaleXYZ = glm::vec3(1.8f, 1.5f, 1.8f); // Scale shape
	XrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(-5.0f, 4.32f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("void"); // Set material
	SetShaderColor(0, 0, 0, 1); //Set color
	//SetShaderTexture("matte_rubber"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Torus - Lower body ring
	scaleXYZ = glm::vec3(2.15f, 2.15f, 0.5f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(-5.0f, 0.5f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderColor(0.1, 0.1, 0.1, 1); //Set color
	//SetShaderTexture("matte_rubber"); // Set texture
	m_basicMeshes->DrawTorusMesh(); // Draw Shape
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█  3  ▄▄   ▀█▀ █▀█ ▄▀█ █▀ █░█   █▀▀ ▄▀█ █▄░█
//█ ░█░ ██▄ █░▀░█     ░░   ░█░ █▀▄ █▀█ ▄█ █▀█   █▄▄ █▀█ █░▀█
//**********************************************************************************
	// Create Tapered Cylinder - Trash can body
	scaleXYZ = glm::vec3(3.5f, 5.4f, 3.5f); // Scale shape
	XrotationDegrees = 180.0f; // Rotate Shape
	YrotationDegrees = -90.0f; // Rotate Shape
	positionXYZ = glm::vec3(4.0f, 5.2f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyish"); // Set material
	SetShaderTexture("can_skin"); // Set texture
	m_basicMeshes->DrawTaperedCylinderMesh(); // Draw Shape

	// Create Cylinder - Black Hole
	scaleXYZ = glm::vec3(3.2f, 0.2f, 3.2f); // Scale shape
	XrotationDegrees = 180.0f; // Rotate Shape
	YrotationDegrees = -90.0f; // Rotate Shape
	positionXYZ = glm::vec3(4.0f, 5.23f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("void"); // Set material
	SetShaderColor(0, 0, 0, 1); //Set color
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Torus - Top ring
	scaleXYZ = glm::vec3(2.96f, 2.96f, 0.5f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	YrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(4.0f, 5.1f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderColor(0.1, 0.1, 0.1, 1); //Set color
	m_basicMeshes->DrawTorusMesh(); // Draw Shape

	// Create Torus - Bottom ring
	scaleXYZ = glm::vec3(1.6f, 1.6f, 0.5f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	YrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(4.0f, 0.08f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderColor(0.1, 0.1, 0.1, 1); //Set color
	m_basicMeshes->DrawTorusMesh(); // Draw Shape
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█   █░█   ▄▄   █▀ █▀▄▀█ ▄▀█ █░░ █░░   █░█░█ █▀▀ █ █▀▀ █░█ ▀█▀
//█ ░█░ ██▄ █░▀░█   ▀▀█   ░░   ▄█ █░▀░█ █▀█ █▄▄ █▄▄   ▀▄▀▄▀ ██▄ █ █▄█ █▀█ ░█░
//**********************************************************************************
	// Create Cylinder - Weight Handle Bar
	scaleXYZ = glm::vec3(0.6f, 5.0f, 0.6f); // Scale shape
	ZrotationDegrees = -90.0f; // Rotate Shape
	positionXYZ = glm::vec3(4.0f, 0.8f, -6.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Box - Left Side weight
	scaleXYZ = glm::vec3(1.1f, 1.0f, 1.6f); // Scale shape
	ZrotationDegrees = -90.0f; // Rotate Shape
	positionXYZ = glm::vec3(3.5f, 0.8f, -6.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Right Side weight
	scaleXYZ = glm::vec3(1.1f, 1.0f, 1.6f); // Scale shape
	ZrotationDegrees = -90.0f; // Rotate Shape
	positionXYZ = glm::vec3(8.5f, 0.8f, -6.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Prism 1 - Right side weight
	scaleXYZ = glm::vec3(1.6f, 1.0f, 0.4f); // Scale shape
	ZrotationDegrees = 90.0f; // Rotate Shape
	XrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(8.5f, 0.8f, -5.65f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawPrismMesh(); // Draw Shape

	// Create Prism 2 - Right side weight
	scaleXYZ = glm::vec3(1.6f, 1.0f, 0.4f); // Scale shape
	ZrotationDegrees = 90.0f; // Rotate Shape
	XrotationDegrees = 180.0f; // Rotate Shape
	positionXYZ = glm::vec3(8.5f, 0.8f, -7.15f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawPrismMesh(); // Draw Shape

	// Create Prism - Left side weight
	scaleXYZ = glm::vec3(1.6f, 1.0f, 0.4f); // Scale shape
	ZrotationDegrees = 90.0f; // Rotate Shape
	XrotationDegrees = 0.0f; // Rotate Shape
	positionXYZ = glm::vec3(3.5f, 0.8f, -5.65f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawPrismMesh(); // Draw Shape

	// Create Prism - Left side weight
	scaleXYZ = glm::vec3(1.6f, 1.0f, 0.4f); // Scale shape
	ZrotationDegrees = 90.0f; // Rotate Shape
	XrotationDegrees = 180.0f; // Rotate Shape
	positionXYZ = glm::vec3(3.5f, 0.8f, -7.15f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("dull"); // Set material
	SetShaderTexture("pink_matte2"); // Set texture
	m_basicMeshes->DrawPrismMesh(); // Draw Shape
//**************************************************************************************************************************************************



//**************************************************************************************************************************************************
//█ ▀█▀ █▀▀ █▀▄▀█   █▀   ▄▄  3 █▀▄ █▀
//█ ░█░ ██▄ █░▀░█   ▄█   ░░    █▄▀ ▄█
//**********************************************************************************
	

	//█▄▄ █▀█ ▀█▀ ▀█▀ █▀█ █▀▄▀█   █▀ █▀▀ █▀█ █▀▀ █▀▀ █▄░█
	//█▄█ █▄█ ░█░ ░█░ █▄█ █░▀░█   ▄█ █▄▄ █▀▄ ██▄ ██▄ █░▀█
	// Create Box - Bottom half frame - Bottom split
	scaleXYZ = glm::vec3(0.2f, 5.0f, 2.0f); // Scale shape
	positionXYZ = glm::vec3(10.0f, 0.1f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby8"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Bottom half - Hidden inside lower half
	scaleXYZ = glm::vec3(0.2f, 4.9f, 1.9f); // Scale shape
	positionXYZ = glm::vec3(10.0f, 0.15f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby6"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Bottom half frame - Top split
	scaleXYZ = glm::vec3(0.15f, 5.0f, 2.0f); // Scale shape
	positionXYZ = glm::vec3(10.0f, 0.3f, -12.4f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby6"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Bottom Screen
	scaleXYZ = glm::vec3(0.2f, 2.5f, 1.4f); // Scale shape
	positionXYZ = glm::vec3(10.0f, 0.3f, -12.2f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Bottom Screen Button Box
	scaleXYZ = glm::vec3(0.2f, 2.5f, 0.2f); // Scale shape
	positionXYZ = glm::vec3(10.0f, 0.32f, -11.55f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1);
	//SetShaderTexture("blue_vase"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape


	//▀█▀ █▀█ █▀█   █▀ █▀▀ █▀█ █▀▀ █▀▀ █▄░█
	//░█░ █▄█ █▀▀   ▄█ █▄▄ █▀▄ ██▄ ██▄ █░▀█
	// Create Box - Top frame
	scaleXYZ = glm::vec3(0.2f, 5.0f, 2.0f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(10.0f, 1.4f, -13.33f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby8"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Top Screen 
	scaleXYZ = glm::vec3(0.2f, 3.2f, 1.6f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(10.0f, 1.2f, -13.32f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Screen Hinge
	scaleXYZ = glm::vec3(0.2f, 4.0f, 0.25f); // Scale shape
	XrotationDegrees = 45.0f; // Rotate Shape
	positionXYZ = glm::vec3(10.0f, 0.4f, -13.28f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shiny"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("blue_vase"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape


	//█░░ █▀▀ █▀▀ ▀█▀   █▀ █ █▀▄ █▀▀   █▄▄ █░█ ▀█▀ ▀█▀ █▀█ █▄░█ █▀
	//█▄▄ ██▄ █▀░ ░█░   ▄█ █ █▄▀ ██▄   █▄█ █▄█ ░█░ ░█░ █▄█ █░▀█ ▄█
	// Create Cylinder - Left side buttons - Joystick holder
	scaleXYZ = glm::vec3(0.35f, 0.1f, 0.35f); // Scale shape
	XrotationDegrees = 90.0f; // Rotate Shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(8.15f, 0.4f, -12.6f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Left side buttons - joystick
	scaleXYZ = glm::vec3(0.25f, 0.1f, 0.25f); // Scale shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(8.15f, 0.45f, -12.6f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	//SetShaderColor(1, 1, 0, 1);
	SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Box - Left side buttons - D pad part 1
	scaleXYZ = glm::vec3(0.5f, 0.2f, 0.15f); // Scale shape
	positionXYZ = glm::vec3(8.15f, 0.32f, -11.8f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Box - Left side buttons - D pad part 2
	scaleXYZ = glm::vec3(0.15f, 0.2f, 0.5f); // Scale shape
	positionXYZ = glm::vec3(8.15f, 0.32f, -11.8f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("porcelaine"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape


	//█▀█ █ █▀▀ █░█ ▀█▀   █▀ █ █▀▄ █▀▀   █▄▄ █░█ ▀█▀ ▀█▀ █▀█ █▄░█ █▀
	//█▀▄ █ █▄█ █▀█ ░█░   ▄█ █ █▄▀ ██▄   █▄█ █▄█ ░█░ ░█░ █▄█ █░▀█ ▄█
	// Create Box - Right side buttons - Home Button
	scaleXYZ = glm::vec3(0.15f, 0.2f, 0.15f); // Scale shape
	positionXYZ = glm::vec3(11.5f, 0.32f, -11.6); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyMaterial"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawBoxMesh(); // Draw Shape

	// Create Cylinder - Right side buttons - Top circle button
	scaleXYZ = glm::vec3(0.14f, 0.1f, 0.14f); // Scale shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(11.9f, 0.4f, -12.65f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyMaterial"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Right side buttons - Bottom circle button
	scaleXYZ = glm::vec3(0.14f, 0.1f, 0.14f); // Scale shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(11.9f, 0.4f, -12.1f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyMaterial"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Right side buttons - Right circle button
	scaleXYZ = glm::vec3(0.14f, 0.1f, 0.14f); // Scale shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(12.15f, 0.4f, -12.37f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyMaterial"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape

	// Create Cylinder - Right side buttons - Left circle button
	scaleXYZ = glm::vec3(0.14f, 0.1f, 0.14f); // Scale shape
	YrotationDegrees = 90.0f; // Rotate Shape
	positionXYZ = glm::vec3(11.65f, 0.4f, -12.37f); // Position shape
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	SetShaderMaterial("shinyMaterial"); // Set material
	SetShaderColor(0.5, 0.5, 0.5, 1); //Set color
	//SetShaderTexture("ruby9"); // Set texture
	m_basicMeshes->DrawCylinderMesh(); // Draw Shape
//**************************************************************************************************************************************************
//**************************************************************************************************************************************************
	





//**************************************************************************************************************************************************
//**************************************************************************************************************************************************
//█░░ █ █▀▀ █░█ ▀█▀   █▄▄ █▀█ ▀▄▀ █▀▀ █▀
//█▄▄ █ █▄█ █▀█ ░█░   █▄█ █▄█ █░█ ██▄ ▄█
//**********************************************************************************
// Visible color cubes tied to the light sources
	glm::vec3 cubeColors[4] = {
		glm::vec3(1.0f, 0.0f, 0.0f),  // Red
		glm::vec3(0.0f, 1.0f, 0.0f),  // Green
		glm::vec3(0.0f, 0.0f, 1.0f),  // Blue
		glm::vec3(1.0f, 1.0f, 0.0f)   // Yellow
	};

	// Loop creating light positioning cubes
	for (int i = 0; i < 4; i++) {
		scaleXYZ = glm::vec3(15.0f, 15.0f, 15.0f); // Cube Scale
		positionXYZ = lightPositions[i]; // Find light positions
		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		SetShaderMaterial("porcelaine"); // Set cube material
		SetShaderColor(cubeColors[i].r, cubeColors[i].g, cubeColors[i].b, 1.0f); // Cube color
		m_basicMeshes->DrawBoxMesh(); // Draw Light Cubes
	}
//**************************************************************************************************************************************************
//**************************************************************************************************************************************************





//**************************************************************************************************************************************************
//**************************************************************************************************************************************************
} //end
//█▀▀ █▄░█ █▀▄   █▀ █▀▀ █▀▀ █▄░█ █▀▀   █▀▄▀█ ▄▀█ █▄░█ ▄▀█ █▀▀ █▀▀ █▀█
//██▄ █░▀█ █▄▀   ▄█ █▄▄ ██▄ █░▀█ ██▄   █░▀░█ █▀█ █░▀█ █▀█ █▄█ ██▄ █▀▄
//**************************************************************************************************************************************************
//**************************************************************************************************************************************************