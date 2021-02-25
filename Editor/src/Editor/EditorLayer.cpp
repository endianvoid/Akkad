#include "EditorLayer.h"
#include "ShaderHandler.h"
#include "Scripting/GameAssemblyHandler.h"

#include "Panels/SceneHierarchyPanel.h"
#include "Panels/PropertyEditorPanel.h"
#include "Panels/NewProjectPanel.h"
#include "Panels/AssetBrowserPanel.h"
#include "Panels/GameViewPanel.h"
#include "Panels/ViewPortPanel.h"
#include "Panels/StartupPanel.h"
#include "Panels/NewScenePanel.h"

#include <Akkad/Application/Application.h>
#include <Akkad/Logging.h>
#include <Akkad/PlatformUtils.h>
#include <Akkad/Input/Input.h>
#include <Akkad/Input/KeyCodes.h>
#include <Akkad/ECS/Entity.h>
#include <Akkad/ECS/Components/Components.h>
#include <Akkad/ECS/SceneSerializer.h>
#include <Akkad/ECS/SceneManager.h>
#include <Akkad/Asset/AssetManager.h>
#include <Akkad/Scripting/LoadedGameAssembly.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.cpp>
#include <IconsForkAwesome.h>

namespace Akkad {
	SharedPtr<Scene> EditorLayer::s_ActiveScene;
	ProjectDescriptor EditorLayer::s_ActiveProject;
	
	EditorLayer::EditorLayer()
	{
		s_ActiveScene = CreateSharedPtr<Scene>();
	}

	void EditorLayer::LoadScene(std::string& filepath)
	{
		PropertyEditorPanel::SetActiveEntity({});
		s_ActiveScene.reset(new Scene());
		SceneSerializer::Deserialize(s_ActiveScene, filepath);

		auto sceneManager = Application::GetSceneManager();
		sceneManager->LoadScene(filepath);
	}

	void EditorLayer::LoadProject()
	{
		PanelManager::Reset();
		PanelManager::AddPanel(new StartupPanel());
	}

	void EditorLayer::SaveActiveScene()
	{
		std::string path = s_ActiveProject.GetAssetsPath().append("scenes/").string() + s_ActiveScene->m_Name + ".AKSCENE";
		SceneSerializer::Serialize(s_ActiveScene, path);
	}

	void EditorLayer::ReloadGameAssembly()
	{
		std::string path = s_ActiveProject.GetProjectDirectory().string();
		path += "GameAssembly/build/GameAssembly";

		GameAssemblyHandler::FreeGameAssembly();
		
		GameAssemblyHandler::LoadGameAssembly(path.c_str());
	}

	void EditorLayer::CompileGameAssembly()
	{
		GameAssemblyHandler::FreeGameAssembly();

		std::string buildpPath = s_ActiveProject.GetProjectDirectory().string();
		buildpPath += "GameAssembly/";

		GameAssemblyHandler::CompileGameAssembly(buildpPath);
		ReloadGameAssembly();

	}

	void EditorLayer::CompileShaders()
	{
		auto& project = GetActiveProject();
		
		for (auto& asset : project.projectData["project"]["Assets"].items())
		{
			std::string assetID = asset.key();
			
			std::string assetType = project.projectData["project"]["Assets"][assetID]["type"];

			if (assetType == "shader")
			{
				std::filesystem::path shaderAbsolutePath = project.GetProjectDirectory();

				std::filesystem::path outputDir = project.GetAssetsPath();
				outputDir.append("compiledSPV/");

				std::string shaderPath = project.projectData["project"]["Assets"][assetID]["path"];
				shaderAbsolutePath.append(shaderPath);

				ShaderHandler::CompileSPV(shaderAbsolutePath, outputDir);

				std::string assetName = project.projectData["project"]["Assets"][assetID]["name"];

				std::string shaderdescPath = "assets/compiledSPV/" + assetName + ".shaderdesc";
				project.projectData["project"]["Assets"][assetID]["shaderdescPath"] = shaderdescPath;

				SaveActiveProject();

			}
		}

	}

	void EditorLayer::OnAttach()
	{
		PanelManager::AddPanel(new StartupPanel());
		ApplyImGuiStyles();
	}

	void EditorLayer::OnDetach()
	{

	}
	

	void EditorLayer::OnUpdate()
	{

		ViewPortPanel* viewport = (ViewPortPanel*)PanelManager::GetPanel("viewport");
		GameViewPanel* gameview = (GameViewPanel*)PanelManager::GetPanel("Game View");

		auto sceneManager = Application::GetSceneManager();

		if (sceneManager->GetActiveScene() != nullptr)
		{
			if (viewport != nullptr && viewport->IsPlaying)
			{
				sceneManager->GetActiveScene()->Update();
			}

			if (viewport != nullptr)
			{
				viewport->RenderScene();
			}

			if (gameview != nullptr)
			{
				gameview->RenderScene();
			}
		}
	}

	void EditorLayer::RenderImGui()
	{
		// ----------- Setup a dockspace -----------------
		auto viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->GetWorkPos());
		ImGui::SetNextWindowSize(viewport->GetWorkSize());
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags host_window_flags = 0;
		host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
		host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		


		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Editor", NULL, host_window_flags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspace_id = ImGui::GetID("DockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f));
		// ------------------------------------------------


		if (!s_ActiveProject.projectData.is_null())
		{
			DrawMainMenuBar();
		}
		
		// Draw panels
		for (auto panel : PanelManager::GetPanels())
		{
			if (panel)
			{
				if (!panel->IsOpen())
				{
					PanelManager::RemovePanel(panel);
					continue;
				}

				panel->DrawImGui();
			}
			
		}

		ImGui::End();

		// Handle keyboard shortcuts
		auto input = Application::GetInputManager();

		if (input->GetKeyDown(AK_KEY_LEFT_CONTROL) & input->GetKeyDown(AK_KEY_S))
		{
			SaveActiveScene();
		}
	}

	void EditorLayer::DrawMainMenuBar()
	{
		bool NewScenepopup = false;
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				ImGui::Separator();
				if (ImGui::BeginMenu("New"))
				{
					if (ImGui::MenuItem("Scene"))
					{
						PanelManager::AddPanel(new NewScenePanel());
					}
					if (ImGui::MenuItem("Project"))
					{
						ProjectDescriptor proj;
						s_ActiveProject = proj;
						LoadProject();
					}
					ImGui::EndMenu();
					
				}

				if (ImGui::MenuItem("Reload Game Assembly"))
				{
					ReloadGameAssembly();
				}

				if (ImGui::MenuItem("Compile Game Assembly"))
				{
					
					CompileGameAssembly();
				}

				if (ImGui::MenuItem("Save", "ctrl + s"))
				{
					SaveActiveScene();
				}

				if (ImGui::BeginMenu("Open"))
				{
					if (ImGui::MenuItem("Project"))
					{
						ProjectDescriptor proj;
						s_ActiveProject = proj;
						LoadProject();
					}

					ImGui::EndMenu();
				}

				if (ImGui::MenuItem("Close"))
				{
					Application::Shutdown();
				}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Create Entity"))
				{
					Entity e = s_ActiveScene->AddEntity();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Graphics"))
			{
				if (ImGui::MenuItem("Recompile shaders"))
				{
					CompileShaders();
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("View"))
			{
				if (ImGui::MenuItem("Hirearchy"))
				{
					SceneHierarchyPanel* panel = new SceneHierarchyPanel();
					PanelManager::AddPanel(panel);
				}

				if (ImGui::MenuItem("Property Editor"))
				{
					PropertyEditorPanel* panel = new PropertyEditorPanel();
					PanelManager::AddPanel(panel);
				}

				if (ImGui::MenuItem("Asset Browser"))
				{
					PanelManager::AddPanel(new AssetBrowserPanel());
				}

				if (ImGui::MenuItem("Game View"))
				{
					PanelManager::AddPanel(new GameViewPanel());
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}
	}

	void EditorLayer::ApplyImGuiStyles() {
		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->AddFontFromFileTTF("res/fonts/Roboto-Medium.ttf", 16.0f);

		static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
		ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
		io.Fonts->AddFontFromFileTTF("res/fonts/forkawesome-webfont.ttf", 16.0f, &icons_config, icons_ranges);

		ImGuiStyle* style = &ImGui::GetStyle();
		ImVec4* colors = style->Colors;

		colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
		colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
		colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
		colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
		colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
		colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
		colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
		colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
		colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
		colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
		colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
		colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
		colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
		colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
		colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
		colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
		colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
		colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
		colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
		colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
		colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
		colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

		style->ChildRounding = 4.0f;
		style->FrameBorderSize = 1.0f;
		style->FrameRounding = 2.0f;
		style->GrabMinSize = 7.0f;
		style->PopupRounding = 2.0f;
		style->ScrollbarRounding = 12.0f;
		style->ScrollbarSize = 13.0f;
		style->TabBorderSize = 1.0f;
		style->TabRounding = 0.0f;
		style->WindowRounding = 4.0f;
	}
}