#pragma once

#include <Akkad/Graphics/RenderPlatform.h>
#include <entt/entt.hpp>

namespace Akkad {

	class Entity;

	class Scene {

	public:
		Scene();
		~Scene();

		void Init();

		void Update();

		Entity AddEntity(std::string tag = "Entity");

	private:
		entt::registry m_Registry;
		SharedPtr<Graphics::Shader> m_colorShader;
		friend class Entity;
		friend class SceneHierarchyPanel;
		friend class PropertyEditorPanel;

	};

}