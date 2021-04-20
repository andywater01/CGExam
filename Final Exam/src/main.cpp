//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

//TODO: New for this tutorial
#include <DirectionalLight.h>
#include <PointLight.h>
#include <UniformBuffer.h>
/////////////////////////////

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
		#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		Shader::sptr simpleDepthShader = Shader::Create();
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_vert.glsl", GL_VERTEX_SHADER);
		simpleDepthShader->LoadShaderPartFromFile("shaders/simple_depth_frag.glsl", GL_FRAGMENT_SHADER);
		simpleDepthShader->Link();
		
		// Load our shaders
		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		// Load our shaders
		Shader::sptr groundShader = Shader::Create();
		groundShader->LoadShaderPartFromFile("shaders/height_displacement_vert.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		groundShader->LoadShaderPartFromFile("shaders/directional_blinn_phong_frag.glsl", GL_FRAGMENT_SHADER);
		groundShader->Link();
		
		// Load our shaders
		Shader::sptr waterShader = Shader::Create();
		waterShader->LoadShaderPartFromFile("shaders/water_vert.glsl", GL_VERTEX_SHADER);
		//Directional Light Shader
		waterShader->LoadShaderPartFromFile("shaders/water_frag.glsl", GL_FRAGMENT_SHADER);
		waterShader->Link();
		
		float displacementIntensity = 11.0f;
		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = groundShader;

		float waterTransparency = 0.8f;

		glm::vec4 deepColor = glm::vec4(0.0, 0.85, 0.65, 1.0);
		glm::vec4 midColor = glm::vec4(0.62, 0.65, 1.0, 1.0);
		glm::vec4 shoreColor = glm::vec4(1.0, 1.0, 1.0, 1.0);
		float shoreCutoff = 0.03f;
		float midCutoff = 0.3f;

		ShaderMaterial::sptr waterMat = ShaderMaterial::Create();
		waterMat->Shader = waterShader;
		waterMat->RenderLayer = 10;



		//Creates our directional Light
		DirectionalLight theSun;
		UniformBuffer directionalLightBuffer;

		//Allocates enough memory for one directional light (we can change this easily, but we only need 1 directional light)
		directionalLightBuffer.AllocateMemory(sizeof(DirectionalLight));
		//Casts our sun as "data" and sends it to the shader
		directionalLightBuffer.SendData(reinterpret_cast<void*>(&theSun), sizeof(DirectionalLight));

		directionalLightBuffer.Bind(0);

		//Basic effect for drawing to
		PostEffect* basicEffect;
		Framebuffer* shadowBuffer;

		//Post Processing Effects
		int activeEffect = 0;
		std::vector<PostEffect*> effects;
		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		ColorCorrectEffect* colorCorrectEffect;
		DepthOfField* depthOfField;
		BloomEffect* bloomEffect;


		
		//Shader Toggles
		bool lightingToggle = false;
		bool ambientToggle = false;
		bool specularToggle = false;
		bool ambientSpecularToggle = false;
		bool customToggle = true;
		bool diffuseRampToggle = false;
		bool specularRampToggle = false;

		bool noLighting = false;
		bool onlyAmbient = false;
		bool onlySpecular = false;
		bool SpecAndAmbient = false;

		bool TexToggle = false;

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 0.35f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);

		
		// We'll add some ImGui controls to control our shader
		BackendHandler::imGuiCallbacks.push_back([&]() {
			if (ImGui::CollapsingHeader("Effect controls"))
			{
				ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1);

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");

					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetIntensity(intensity);
					}
				}
				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Color Correct Effect");
					
					ColorCorrectEffect* temp = (ColorCorrectEffect*)effects[activeEffect];
					static char input[BUFSIZ];
					ImGui::InputText("Lut File to Use", input, BUFSIZ);

					if (ImGui::Button("SetLUT", ImVec2(200.0f, 40.0f)))
					{
						temp->SetLUT(LUT3D(std::string(input)));
					}
				}
				/*if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Depth of Field");

					DepthOfField* temp = (DepthOfField*)effects[activeEffect];
					float depth = temp->GetDepth();
					if (ImGui::SliderFloat("Depth", &depth, 0.0f, 1.0f))
					{
						temp->SetDepth(depth);
					}

					float intensity = temp->GetThreshold();

					if (ImGui::SliderFloat("Threshold", &intensity, 1.0f, 0.0f))
					{
						temp->SetThreshold(intensity);
					}
				}*/
				if (activeEffect == 3)
				{
					ImGui::Text("Active Effect: Depth of Field");

					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					float threshold = temp->GetThreshold();
					if (ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f))
					{
						temp->SetThreshold(threshold);
					}
					
					float passes = temp->GetPasses();
					if (ImGui::SliderFloat("Depth", &passes, 0.0f, 50.0f))
					{
						temp->SetPasses(passes);
					}
					
					
				}
			}
			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Direction/Position", glm::value_ptr(theSun._lightDirection), 0.01f, -10.0f, 10.0f)) 
				{
					directionalLightBuffer.SendData(reinterpret_cast<void*>(&theSun), sizeof(DirectionalLight));
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(theSun._lightCol)))
				{
					directionalLightBuffer.SendData(reinterpret_cast<void*>(&theSun), sizeof(DirectionalLight));
				}
			}
			if (ImGui::CollapsingHeader("World Showcase Settings"))
			{
				ImGui::Text("Ground Controls");

				if (ImGui::SliderFloat("Height Intensity", &displacementIntensity, 1.0f, 100.0f))
				{
					grassMat->Set("u_HeightIntensity", displacementIntensity);
				}

				ImGui::Separator();
				ImGui::Text("Water Controls");

				if (ImGui::SliderFloat("Water Transparency", &waterTransparency, 0.01f, 1.0f))
				{
					waterMat->Set("u_waterTransparency", waterTransparency);
				}

				if (ImGui::ColorEdit4("Deep Color", glm::value_ptr(deepColor)))
				{
					waterMat->Set("u_deepestColor", deepColor);
				}
				if (ImGui::ColorEdit4("Mid Color", glm::value_ptr(midColor)))
				{
					waterMat->Set("u_midColor", midColor);
				}
				if (ImGui::ColorEdit4("Shore Color", glm::value_ptr(shoreColor)))
				{
					waterMat->Set("u_shoreColor", shoreColor);
				}

				if (ImGui::SliderFloat("Shore Cutoff", &shoreCutoff, 0.0f, midCutoff))
				{
					waterMat->Set("u_shoreCutoff", shoreCutoff);
				}
				if (ImGui::SliderFloat("Mid Cutoff", &midCutoff, shoreCutoff, 1.0))
				{
					waterMat->Set("u_midCutoff", midCutoff);
				}

			}
			if (ImGui::CollapsingHeader("Light Settings"))
			{
				if (ImGui::Button("No Lighting"))
				{
					shader->SetUniform("u_AmbientLightStrength", 0.0f);
					shader->SetUniform("u_AmbientStrength", 0.0f);
					shader->SetUniform("u_SpecularLightStrength", 0.0f);
					shader->SetUniform("u_LightAttenuationConstant", 1.0f);

					waterShader->SetUniform("u_AmbientLightStrength", 0.0f);
					waterShader->SetUniform("u_AmbientStrength", 0.0f);
					waterShader->SetUniform("u_SpecularLightStrength", 0.0f);
					waterShader->SetUniform("u_LightAttenuationConstant", 1.0f);


				}
				if (ImGui::Button("Ambient"))
				{
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
					shader->SetUniform("u_AmbientStrength", ambientPow);
					shader->SetUniform("u_SpecularLightStrength", 0.0f);
					shader->SetUniform("u_LightAttenuationConstant", 1.0f);


				}
				if (ImGui::Button("Specular"))
				{
					
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
					shader->SetUniform("u_AmbientLightStrength", 0.0f);
					shader->SetUniform("u_AmbientStrength", 0.0f);
					shader->SetUniform("u_LightAttenuationConstant", 0.0f);
					
				}
				if (ImGui::Button("Ambient + Specular"))
				{
					
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
					shader->SetUniform("u_AmbientStrength", ambientPow);
					shader->SetUniform("u_LightAttenuationConstant", 0.0f);
					shader->SetUniform("uThreshold", 0.0f);
				
				}
				if (ImGui::Button("Ambient + Specular + Depth of Field"))
				{

					/*shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
					shader->SetUniform("u_AmbientStrength", ambientPow);
					shader->SetUniform("u_LightAttenuationConstant", 0.0f);
					shader->SetUniform("uThreshold", 0.0f);*/

				}
				if (ImGui::Button("Toggle Textures"))
				{

					TexToggle = !TexToggle;

				}
				
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");
		
			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

		#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		///////////////////////////////////// Texture Loading //////////////////////////////////////////////////
		#pragma region Texture
		
		// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/BeachSand.png");
		Texture2D::sptr PalmTree = Texture2D::LoadFromFile("images/PalmTreeTexture.png");
		Texture2D::sptr heightMap = Texture2D::LoadFromFile("images/groundHeightMap.png");
		Texture2D::sptr water = Texture2D::LoadFromFile("images/Water.png");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		Texture2D::sptr TreasureChest = Texture2D::LoadFromFile("images/TreasureChestTexture.png");
		Texture2D::sptr White = Texture2D::LoadFromFile("images/White.png");
		LUT3D testCube("cubes/BrightenedCorrection.cube");
		
		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg"); 

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();  
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

		#pragma endregion
		//////////////////////////////////////////////////////////////////////////////////////////
		
		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
		#pragma region Scene Generation
		
		// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();  
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f); 
		

		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("s_Height", heightMap);
		grassMat->Set("u_HeightIntensity", displacementIntensity);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		waterMat->Set("s_Diffuse", water);
		waterMat->Set("s_Specular", noSpec);
		waterMat->Set("u_waterTransparency", waterTransparency);
		waterMat->Set("u_deepestColor", deepColor);
		waterMat->Set("u_midColor", midColor);
		waterMat->Set("u_shoreColor", shoreColor);
		waterMat->Set("u_shoreCutoff", shoreCutoff);
		waterMat->Set("u_midCutoff", midCutoff);
		waterMat->Set("u_Shininess", 8.0f);
		waterMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = shader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = shader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);


		ShaderMaterial::sptr PalmTreeMat = ShaderMaterial::Create();
		PalmTreeMat->Shader = shader;
		PalmTreeMat->Set("s_Diffuse", PalmTree);
		PalmTreeMat->Set("s_Specular", noSpec);
		PalmTreeMat->Set("u_Shininess", 8.0f);
		PalmTreeMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr TreasureChestMat = ShaderMaterial::Create();
		TreasureChestMat->Shader = shader;
		TreasureChestMat->Set("s_Diffuse", TreasureChest);
		TreasureChestMat->Set("s_Specular", noSpec);
		TreasureChestMat->Set("u_Shininess", 8.0f);
		TreasureChestMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr WhiteMat = ShaderMaterial::Create();
		WhiteMat->Shader = shader;
		WhiteMat->Set("s_Diffuse", White);
		WhiteMat->Set("s_Specular", noSpec);
		WhiteMat->Set("u_Shininess", 8.0f);
		WhiteMat->Set("u_TextureMix", 0.0f);

		VertexArrayObject::sptr planeVAO = ObjLoader::LoadFromFile("models/plane.obj");

		GameObject obj1 = scene->CreateEntity("Ground"); 
		{
			
			obj1.emplace<RendererComponent>().SetMesh(planeVAO).SetMaterial(grassMat).SetCastShadow(false);
		}

		GameObject obj2 = scene->CreateEntity("Ground");
		{
			
			obj2.emplace<RendererComponent>().SetMesh(planeVAO).SetMaterial(waterMat).SetCastShadow(false).SetIsTransparent(true);
		}
		
		GameObject obj3 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/monkey_quads.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj3.get<Transform>().SetLocalPosition(0.0f, 0.0f, -5.0f);
			obj3.get<Transform>().SetLocalRotation(0.0f, 0.0f, -90.0f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj3);
		}

		VertexArrayObject::sptr PalmTreeVAO = ObjLoader::LoadFromFile("models/PalmTreeOBJ.obj");

		GameObject PalmTree1 = scene->CreateEntity("palmtree");
		{
			
			PalmTree1.emplace<RendererComponent>().SetMesh(PalmTreeVAO).SetMaterial(PalmTreeMat).SetCastShadow(false);
			PalmTree1.get<Transform>().SetLocalPosition(-4.4f, -3.0f, 1.2f);
			PalmTree1.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PalmTree1.get<Transform>().SetLocalScale(0.4f, 0.4f, 0.4f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PalmTree1);
		}

		GameObject PalmTree2 = scene->CreateEntity("palmtree");
		{

			PalmTree2.emplace<RendererComponent>().SetMesh(PalmTreeVAO).SetMaterial(PalmTreeMat).SetCastShadow(false);
			PalmTree2.get<Transform>().SetLocalPosition(1.0f, 10.0f, 1.2f);
			PalmTree2.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			PalmTree2.get<Transform>().SetLocalScale(0.4f, 0.4f, 0.4f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(PalmTree2);
		}

		GameObject Chest = scene->CreateEntity("TreasureChest");
		{
			VertexArrayObject::sptr TreasureChestVAO = ObjLoader::LoadFromFile("models/TreasureChest.obj");
			Chest.emplace<RendererComponent>().SetMesh(TreasureChestVAO).SetMaterial(TreasureChestMat).SetCastShadow(false);
			Chest.get<Transform>().SetLocalPosition(0.5f, -2.25f, 1.0f);
			Chest.get<Transform>().SetLocalRotation(90.0f, 0.0f, -90.0f);
			Chest.get<Transform>().SetLocalScale(0.4f, 0.4f, 0.4f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(Chest);
		}
		

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}

		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);

		int shadowWidth = 4096;
		int shadowHeight = 4096;

		GameObject shadowBufferObject = scene->CreateEntity("Shadow Buffer");
		{
			shadowBuffer = &shadowBufferObject.emplace<Framebuffer>();
			shadowBuffer->AddDepthTarget();
			shadowBuffer->Init(shadowWidth, shadowHeight);
		}


		GameObject framebufferObject = scene->CreateEntity("Basic Effect");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
			sepiaEffect->SetIntensity(0.0f);
		}
		effects.push_back(sepiaEffect);

		GameObject greyscaleEffectObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyscaleEffectObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject colorCorrectEffectObject = scene->CreateEntity("Colour Correction");
		{
			colorCorrectEffect = &colorCorrectEffectObject.emplace<ColorCorrectEffect>();
			colorCorrectEffect->Init(width, height);
		}
		effects.push_back(colorCorrectEffect);

		/*GameObject depthOfFieldObject = scene->CreateEntity("Depth of Field");
		{
			depthOfField = &depthOfFieldObject.emplace<DepthOfField>();
			depthOfField->Init(width, height);
			
		}
		effects.push_back(depthOfField);*/

		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			bloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			bloomEffect->Init(width, height);

		}
		effects.push_back(bloomEffect);

		#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////
		
		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;  
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();
			
			GameObject skyboxObj = scene->CreateEntity("skybox");  
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat).SetCastShadow(false);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(obj3);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);
			time.totalTime += time.DeltaTime;

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			waterMat->Set("u_Time", time.totalTime);

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			if (TexToggle == false)
			{
				stoneMat->Set("s_Diffuse", stone);
				grassMat->Set("s_Diffuse", grass);
				waterMat->Set("s_Diffuse", water);
				boxMat->Set("s_Diffuse", box);
				simpleFloraMat->Set("s_Diffuse", simpleFlora);
				PalmTreeMat->Set("s_Diffuse", PalmTree);
				TreasureChestMat->Set("s_Diffuse", TreasureChest);
			}
			else if (TexToggle == true)
			{
				stoneMat->Set("s_Diffuse", White);
				grassMat->Set("s_Diffuse", White);
				waterMat->Set("s_Diffuse", White);
				boxMat->Set("s_Diffuse", White);
				simpleFloraMat->Set("s_Diffuse", White);
				PalmTreeMat->Set("s_Diffuse", White);
				TreasureChestMat->Set("s_Diffuse", White);
			}
			


			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
			});

			// Clear the screen
			basicEffect->Clear();
			/*greyscaleEffect->Clear();
			sepiaEffect->Clear();*/
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear();
			}
			shadowBuffer->Clear();


			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
			});
			
			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			//Set up light space matrix
			glm::mat4 lightProjectionMatrix = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, -30.0f, 30.0f);
			glm::mat4 lightViewMatrix = glm::lookAt(glm::vec3(-theSun._lightDirection), glm::vec3(), glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightSpaceViewProj = lightProjectionMatrix * lightViewMatrix;
						
			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;
				
				return false;
			});
			
			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			glViewport(0, 0, shadowWidth, shadowHeight);
			shadowBuffer->Bind();

			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// Render the mesh
				if (renderer.CastShadows)
				{
					BackendHandler::RenderVAO(simpleDepthShader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
				}
			});

			shadowBuffer->Unbind();


			glfwGetWindowSize(BackendHandler::window, &width, &height);

			glViewport(0, 0, width, height);
			basicEffect->BindBuffer(0);

			waterMat->Set("u_WindowWidth", (float)width);
			waterMat->Set("u_WindowHeight", (float)height);


			// Iterate over the render group components and draw them
			renderGroup.each( [&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}

				if (renderer.IsTransparent)
				{
					basicEffect->BindDepthAsTexture(0, 29);
					glDepthMask(GL_FALSE);
				}

				shadowBuffer->BindDepthAsTexture(30);
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform, lightSpaceViewProj);
			
				if (renderer.IsTransparent)
				{
					basicEffect->UnbindTexture(29);
					glDepthMask(GL_TRUE);
				}
			});

			shadowBuffer->UnbindTexture(30);
			basicEffect->UnbindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			
			effects[activeEffect]->DrawToScreen();
			
			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		directionalLightBuffer.Unbind(0);

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}	

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}