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

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials() {

	// ---------------- BOOK MATERIAL ----------------
	OBJECT_MATERIAL bookMaterial;
	bookMaterial.ambientColor = glm::vec3(0.2f, 0.1f, 0.05f);
	bookMaterial.ambientStrength = 0.4f;
	bookMaterial.diffuseColor = glm::vec3(0.6f, 0.3f, 0.1f);
	bookMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	bookMaterial.shininess = 10.0f;
	bookMaterial.tag = "book";
	m_objectMaterials.push_back(bookMaterial);

	
	// ---------------- DESK MATERIAL (REFLECTIVE) ----------------
	OBJECT_MATERIAL deskMaterial;
	deskMaterial.ambientColor = glm::vec3(0.25f, 0.25f, 0.25f);     // brighter ambient
	deskMaterial.ambientStrength = 0.7f;                            // stronger light pickup
	deskMaterial.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);         // allow texture to dominate
	deskMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);        // strong reflections
	deskMaterial.shininess = 64.0f;                                 // sharper highlight
	deskMaterial.tag = "desk";
	m_objectMaterials.push_back(deskMaterial);

	// ---------------- CUP MATERIAL ----------------
	OBJECT_MATERIAL cupMaterial;
	cupMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cupMaterial.ambientStrength = 0.3f;
	cupMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	cupMaterial.specularColor = glm::vec3(1.0f, 1.0f, 1.0f); // reflective glass look
	cupMaterial.shininess = 95.0f;
	cupMaterial.tag = "cup";
	m_objectMaterials.push_back(cupMaterial);

	// ---------------- NOTEBOOK MATERIAL ----------------
	OBJECT_MATERIAL notebookMaterial;
	notebookMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	notebookMaterial.ambientStrength = 0.4f;
	notebookMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.7f); // bluish notebook
	notebookMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.4f);
	notebookMaterial.shininess = 18.0f;
	notebookMaterial.tag = "notebook";
	m_objectMaterials.push_back(notebookMaterial);

	// ---------------- NOTEBOOK RING MATERIAL ----------------
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	metalMaterial.specularColor = glm::vec3(0.7f, 0.7f, 0.7f);
	metalMaterial.shininess = 42.0;
	metalMaterial.tag = "metal";

	m_objectMaterials.push_back(metalMaterial);


	// ---------------- MECHANICAL PENCIL MATERIAL ----------------
	OBJECT_MATERIAL mechPencilMaterial;
	mechPencilMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.15f);   // subtle blue tint
	mechPencilMaterial.ambientStrength = 0.4f;
	mechPencilMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.8f);      // blue pencil body
	mechPencilMaterial.specularColor = glm::vec3(0.4f, 0.4f, 0.4f);     // slight shine
	mechPencilMaterial.shininess = 32.0f;                               // smooth highlight
	mechPencilMaterial.tag = "mechpencil";
	m_objectMaterials.push_back(mechPencilMaterial);


	// ---------------- ERASER MATERIAL ----------------
	OBJECT_MATERIAL eraserMaterial;
	eraserMaterial.ambientColor = glm::vec3(0.3f, 0.15f, 0.15f);        // soft pinkish tone
	eraserMaterial.ambientStrength = 0.5f;
	eraserMaterial.diffuseColor = glm::vec3(1.0f, 0.6f, 0.6f);          // pink rubber
	eraserMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);         // almost no shine
	eraserMaterial.shininess = 5.0f;                                    // very matte
	eraserMaterial.tag = "eraser";
	m_objectMaterials.push_back(eraserMaterial);

}



 /***********************************************************
  *  SetupSceneLights()
  *
  *  This method is called to add and configure the light
  *  sources for the 3D scene.
  ***********************************************************/
void SceneManager::SetupSceneLights()
{
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// ============================================================
	// 1. DIRECTIONAL LIGHT — Slightly colored, soft, room-filling
	// ============================================================
	// Gives a gentle cool tint and fills shadows uniformly.
	m_pShaderManager->setVec3Value("directionalLight.direction", -0.2f, -1.0f, -0.3f);
	m_pShaderManager->setVec3Value("directionalLight.ambient", 0.25f, 0.22f, 0.30f);    // colored ambient
	m_pShaderManager->setVec3Value("directionalLight.diffuse", 0.55f, 0.50f, 0.70f);    // soft bluish tint
	m_pShaderManager->setVec3Value("directionalLight.specular", 0.25f, 0.25f, 0.35f);
	m_pShaderManager->setBoolValue("directionalLight.bActive", true);

	// ============================================================
	// 2. POINT LIGHT — bright white overhead fill (primary light)
	// ============================================================
	// Illuminates everything from above and slightly forward.
	m_pShaderManager->setVec3Value("pointLights[0].position", 0.0f, 7.0f, 3.0f);

	m_pShaderManager->setVec3Value("pointLights[0].ambient", 0.20f, 0.20f, 0.20f);
	m_pShaderManager->setVec3Value("pointLights[0].diffuse", 0.95f, 0.95f, 0.90f);
	m_pShaderManager->setVec3Value("pointLights[0].specular", 1.0f, 1.0f, 1.0f);

	m_pShaderManager->setFloatValue("pointLights[0].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[0].linear", 0.045f);      // larger reach
	m_pShaderManager->setFloatValue("pointLights[0].quadratic", 0.015f);   // smoother falloff

	m_pShaderManager->setBoolValue("pointLights[0].bActive", true);

	// ============================================================
	// 3. Secondary Fill Light — soft warm point light (optional but helpful)
	// ============================================================
	// Eliminates dark sides when moving the camera around objects.
	m_pShaderManager->setVec3Value("pointLights[1].position", -6.0f, 3.5f, 2.5f);

	m_pShaderManager->setVec3Value("pointLights[1].ambient", 0.10f, 0.07f, 0.05f);
	m_pShaderManager->setVec3Value("pointLights[1].diffuse", 0.55f, 0.40f, 0.25f); // warm tint
	m_pShaderManager->setVec3Value("pointLights[1].specular", 0.25f, 0.20f, 0.15f);

	m_pShaderManager->setFloatValue("pointLights[1].constant", 1.0f);
	m_pShaderManager->setFloatValue("pointLights[1].linear", 0.09f);
	m_pShaderManager->setFloatValue("pointLights[1].quadratic", 0.032f);

	m_pShaderManager->setBoolValue("pointLights[1].bActive", true);

	// ============================================================
	// Disable unused lights if your shader expects four
	// ============================================================
	m_pShaderManager->setBoolValue("pointLights[2].bActive", false);
	m_pShaderManager->setBoolValue("pointLights[3].bActive", false);
}




/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the JPG photo textures into memory to support 3D scene
 *  rendering.
 ***********************************************************/
void SceneManager::LoadSceneTextures() {

	bool bReturn = false;

	//jpg image for the desk
	bReturn = CreateGLTexture("Photos/textures/black_top_vinyl.jpg","desk");

	//jpg image for the cup
	bReturn = CreateGLTexture("Photos/textures/cup.jpg", "cup");
	//jpg image for the cup rim
	bReturn = CreateGLTexture("Photos/textures/rim.jpg", "cup_rim");
	//jpg image for the french book
	bReturn = CreateGLTexture("Photos/textures/french.jpg", "french");
	//jpg image for the notebook
	bReturn = CreateGLTexture("Photos/textures/paper.jpg", "paper");

	//jpg image for the notebook rings
	bReturn = CreateGLTexture("Photos/textures/stainless.jpg", "metal");

	//jpg imagw for the mech pencil body.
	bReturn = CreateGLTexture("Photos/textures/mech_body.jpg", "body");

	//jpeg image for the mech pencil pointy tip.
	CreateGLTexture("Photos/textures/point.jpg", "point");

	//jpeg image for the mech pencil eraser.
	CreateGLTexture("Photos/textures/white_eraser.jpg", "eraser");

	//jpeg image for the mech pencil clip.
	CreateGLTexture("Photos/textures/clip.jpg", "clip");

	//jpeg image for the pink eraser.
	CreateGLTexture("Photos/textures/eraser.jpg", "pink_eraser");

	BindGLTextures();

}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	LoadSceneTextures();

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadTorusMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadTaperedCylinderMesh(); // Tapered cylinder for pencil tip
	m_basicMeshes->LoadConeMesh();
	//m_basicMeshes->DrawSphereMesh();
	
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/

	

	// --------------------------------------------------------------
	// Desk Surface (Plane)
	// --------------------------------------------------------------

	// Set the XYZ scale for the desk plane
	// Wider and deeper to look like a desk surface
	scaleXYZ = glm::vec3(16.0f, 0.75f, 9.0f);


	// Desk lies flat on the ground, no rotation needed
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Center the desk in the scene
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// Store the transformation before drawing
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	// Grayish desk surface
	// (0.6, 0.6, 0.6, 1.0) ? soft gray
	//SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f);
	
	//setting the texture for the plane.
	SetShaderTexture("desk");
	SetShaderMaterial("desk");

	// Draw the plane that acts as the desk
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/
	
	//creates the coffee cup
	DrawCup();

	//creates the french book.
	DrawFrenchBook();

	//creates the notebook.
	DrawNoteBook();

	//creates the mechanical pencil.
	DrawMechPencil();

	//creates the eraser.
	DrawEraser();

	
}

// --------------------------------------------------------------
// DrawCup()
// Builds a 3D coffee cup using three basic shapes:
//   - Cylinder : cup body
//   - Torus    : handle
//   - Torus    : rim (flattened)
// --------------------------------------------------------------
void SceneManager::DrawCup()
{
	// Transformation variables (re-used for each shape)
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// ----------------------------------------------------------
	// Cup Body (Cylinder)
	// ----------------------------------------------------------
	scaleXYZ = glm::vec3(1.0f, 2.0f, 1.0f);    // Tall cylinder
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-2.5f, 0.0f, -1.0f);

	SetTransformations(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.6f, 1.0f, 0.6f, 1.0f);    // Light green

	//setting the texture for the body of the cup.
	SetShaderTexture("cup");
	SetShaderMaterial("cup");

	m_basicMeshes->DrawCylinderMesh(false,true,true);


	// ----------------------------------------------------------
	// Cup Handle (Torus)
	// ----------------------------------------------------------
	scaleXYZ = glm::vec3(0.35f, 0.6f, 0.5f);   // Thin, tall torus
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-1.4f, 1.0f, -1.0f); // Positioned at the side

	SetTransformations(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.7f, 1.0f, 0.7f, 1.0f);    // Slightly lighter green
	
	//setting the texture for the handle of the cup.
	SetShaderTexture("cup");
	SetShaderMaterial("cup");

	m_basicMeshes->DrawTorusMesh();


	// ----------------------------------------------------------
	// Cup Rim (Flattened Torus)
	// ----------------------------------------------------------

	 

	scaleXYZ = glm::vec3(0.8f, 0.8f, 0.8f);

	
	XrotationDegrees = -90.0f;
	YrotationDegrees = 0.0f;   
	ZrotationDegrees = 0.0f;

	// Cylinder position: (-2.5, 0.0, -1.0)
	// Cylinder height scale = 2.0 ? top is at Y = 1.0
	positionXYZ = glm::vec3(-2.5f, 1.85f, -1.0f);

	SetTransformations(scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	//SetShaderColor(0.5f, 0.8f, 0.5f, 1.0f);
	
	//setting the texture for the rim of the cup.
	SetShaderTexture("cup_rim");
	SetShaderMaterial("cup");

	m_basicMeshes->DrawTorusMesh();
}

// --------------------------------------------------------------
// DrawFrenchBook()
// Builds a 3D book using one basic shape:
//   - Box : French Book
// --------------------------------------------------------------

void SceneManager::DrawFrenchBook() {

	// Transformation variables 
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	//*************************************************************************/
	// French Book (Box)
	//*************************************************************************/

	// Set the scale for the book.
	scaleXYZ = glm::vec3(5.0f, 1.0f, 5.0f);

	// rotational variables.
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// french book position.
	positionXYZ = glm::vec3(-6.0f, 0.5f, 5.0f);



	// Apply transformations
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	// blue color
	//SetShaderColor(0.0f, 0.447f, 0.733f, 1.0f);
	
	//setting the texture for the french book.
	SetShaderTexture("french");
	SetShaderMaterial("book");

	
	// Draw the box
	m_basicMeshes->DrawBoxMesh();
	//*************************************************************************************************/

}

void SceneManager::DrawNoteBook() {
	//*************************************************************************/
	// Notebook (Box) Left Side
	//*************************************************************************/

	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// Left cover
	scaleXYZ = glm::vec3(6.0f, 1.0f, 6.0f);
	positionXYZ = glm::vec3(-0.4f, 0.0f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);

	//setting the texture for the left side of the notebook.
	SetShaderTexture("paper");
	SetShaderMaterial("notebook");

	m_basicMeshes->DrawBoxMesh();


	//*************************************************************************/
	// Notebook (Box) Right Side
	//*************************************************************************/

	scaleXYZ = glm::vec3(6.0f, 1.5f, 6.0f);
	positionXYZ = glm::vec3(5.5f, 0.0f, 4.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);
	
	//setting the texture for right side of the notebook.
	SetShaderTexture("notebook");
	SetShaderMaterial("notebook");

	m_basicMeshes->DrawBoxMesh();

//*************************************************************************/
// Notebook (Torus) Rings
//*************************************************************************/

// Ring size
	scaleXYZ = glm::vec3(0.3f, 0.3f, 0.3f);

	// First ring position
	float startX = 2.6f;      // near the spine
	float y = 0.75f;            // height above notebook
	float z = 1.5f;            // depth

	int ringCount = 8;
	float spacing = 0.75f;

	for (int i = 0; i < ringCount; i++) {
		positionXYZ = glm::vec3(startX, y, z + i * spacing);

		// Rotate so ring faces the screen
		XrotationDegrees = 0.0f;
		YrotationDegrees = 0.0f;
		ZrotationDegrees = 90.0f;

		SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
		//SetShaderColor(0.8f, 0.8f, 0.8f, 1.0f);

		//setting the tecture for the metal rings for the binder of notebook.
		SetShaderTexture("metal");
		SetShaderMaterial("metal");

		m_basicMeshes->DrawTorusMesh();
	}

}

void SceneManager::DrawMechPencil()
{
	glm::vec3 scaleXYZ;                      // Declare variable for scaling the pencil mesh
	glm::vec3 positionXYZ;                   // Declare variable for positioning the pencil mesh
	float XrotationDegrees = 0.0f;          // Rotate cylinder 90° around X to lie horizontally
	float YrotationDegrees = 0.0f;           // No rotation around Y axis
	float ZrotationDegrees = 0.0f;           // No rotation around Z axis

	//************************************************************************************************************
	
	scaleXYZ = glm::vec3(0.1f, 5.0f, 0.1f);    // Tall cylinder
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 90.0f;
	positionXYZ = glm::vec3(7.0f, 0.1f, -2.0f);

	SetTransformations(                       // Apply scale, rotation, and position to pencil
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ
	);

	//SetShaderColor(0.0f, 0.0f, 1.0f, 1.0f);  


	//setting the texture for the body of the mechnical pencil.
	SetShaderTexture("body");
	SetShaderMaterial("mechpencil");

	m_basicMeshes->DrawCylinderMesh();       // Draw the pencil body using a cylinder mesh

	//*************************************************************************/
	// Pointy Tip (Tapered Cylinder with Cone)
	//*************************************************************************/
	
	scaleXYZ = glm::vec3(0.1f, 0.1f, 0.1f);   
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -270.0f;
	positionXYZ = glm::vec3(2.0f, 0.1f, -2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f);

	//setting the texture for the body of the mechnical tip.
	SetShaderTexture("point");
	SetShaderMaterial("mechpencil");

	m_basicMeshes->DrawTaperedCylinderMesh();


	//*****************************************************************************
	// Cone
	//*****************************************************************************
	scaleXYZ = glm::vec3(0.05f, 0.05f, 0.05f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -270.0f;
	positionXYZ = glm::vec3(1.90f, 0.1f, -2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.0f, 1.0f, 1.0f, 1.0f);

	//setting the texture for the body of the mechnical tip.
	SetShaderTexture("body");
	SetShaderMaterial("mechpencil");

	m_basicMeshes->DrawConeMesh();

	//*************************************************************************/
	// Eraser Tip (Cylinder)
	//*************************************************************************/
	scaleXYZ = glm::vec3(0.1f, 0.2f, 0.1f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = -270.0f;
	positionXYZ = glm::vec3(7.19f, 0.1f, -2.0f);

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(1.0f, 1.0f, 1.0f, 1.0f);

	//setting the texture for the body of the mechnical pencil.
	SetShaderTexture("eraser");
	SetShaderMaterial("eraser");

	m_basicMeshes->DrawCylinderMesh();



	//*************************************************************************/
	// Plastic Clip (Box)
	//*************************************************************************/

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	scaleXYZ = glm::vec3(0.6f, 0.15f, 0.1f);
	positionXYZ = glm::vec3(6.0f, 0.2f, -1.9f); // Slightly higher

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);

	//SetShaderColor(0.0f, 0.0f, 0.0f, 1.0f);  // Bright red to see clearly

	SetShaderTexture("clip");
	SetShaderMaterial("mechpencil");

	m_basicMeshes->DrawBoxMesh();
}


void SceneManager::DrawEraser() {

	// Transformation variables 
	glm::vec3 scaleXYZ;
	glm::vec3 positionXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;

	// =======================================================
	// ERASER (75% larger)
	// =======================================================

	// ---------------------------------------------
	// MAIN ERASER BODY (Box)
	// ---------------------------------------------
	scaleXYZ = glm::vec3(0.7875f, 0.4025f, 0.4025f);   // +75% size increase
	positionXYZ = glm::vec3(11.0f, 0.1f, 1.0f);       // center stays same

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 0.6f, 0.6f, 1.0f);


	//setting the texture for the body of the pink eraser.
	SetShaderTexture("pink_eraser");
	SetShaderMaterial("eraser");

	m_basicMeshes->DrawBoxMesh();

	// ---------------------------------------------
	// LEFT CHAMFER
	// ---------------------------------------------
	scaleXYZ = glm::vec3(0.2625f, 0.4025f, 0.4025f);   // +75% scale
	positionXYZ = glm::vec3(10.61f, 0.1f, 1.0f);       // moved outward

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 0.55f, 0.55f, 1.0f);

	//setting the texture for the body of the pink eraser.
	SetShaderTexture("pink_eraser");
	SetShaderMaterial("eraser");

	m_basicMeshes->DrawPrismMesh();

	// ---------------------------------------------
	// RIGHT CHAMFER
	// ---------------------------------------------
	scaleXYZ = glm::vec3(0.2625f, 0.4025f, 0.4025f);   // +75% scale
	YrotationDegrees = 180.0f;
	positionXYZ = glm::vec3(11.39f, 0.1f, 1.0f);       // moved outward

	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	//SetShaderColor(1.0f, 0.55f, 0.55f, 1.0f);
	
	//setting the texture for the body of the pink eraser.
	SetShaderTexture("pink_eraser");
	SetShaderMaterial("eraser");

	m_basicMeshes->DrawPrismMesh();
}




