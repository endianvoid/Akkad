#include "Scene.h"
#include "Entity.h"
#include "Serializers/SceneSerializer.h"
#include "Serializers/InstantiableEntitySerializer.h"

#include "Akkad/Logging.h"
#include "Akkad/Application/Application.h"
#include "Akkad/Input/Input.h"
#include "Akkad/Graphics/Renderer2D.h"
#include "Akkad/Asset/AssetManager.h"
#include "Akkad/Graphics/SortingLayer2D.h"
#include "Akkad/Application/TimeManager.h"

#include "Components/Components.h"

namespace Akkad {
	using namespace Graphics;
	Scene::Scene()
	{
		FrameBufferDescriptor pickingBufferDescriptor;
		pickingBufferDescriptor.width = 800;
		pickingBufferDescriptor.height = 800;
		pickingBufferDescriptor.ColorAttachmentFormat = TextureFormat::R32_FLOAT;
		pickingBufferDescriptor.usesRenderBuffer = true;
		if (Application::GetRenderPlatform() != nullptr)
		{
			m_PickingBuffer = Application::GetRenderPlatform()->CreateFrameBuffer(pickingBufferDescriptor);
		}
	}

	Scene::~Scene()
	{
		m_PhysicsWorld2D.Clear();
	}

	void Scene::Start()
	{
		Entity activeContainerEntity = GetGuiContainer();
		if (activeContainerEntity.IsValid() && activeContainerEntity.HasComponent<GUIContainerComponent>())
		{
			auto& container = activeContainerEntity.GetComponent<GUIContainerComponent>();
			container.container.SetScreenSize(m_ViewportSize);
			
		}

		// Initialize physics
		{
			m_PhysicsWorld2D.SetContactListener(&m_PhysicsListener2D);
			m_PhysicsWorld2D.SetDebugDraw(&m_PhysicsDebugDraw2D);
			m_PhysicsWorld2D.Clear();

			// Init bodies
			{
				auto view = m_Registry.view<TransformComponent,RigidBody2dComponent>();
				
				for (auto entity : view)
				{
					InitilizePhysicsBodies2D({ entity, this });
				
				}
			}

			// Init joints
			{
				auto view = m_Registry.view<HingeJoint2DComponent>();

				for (auto entity : view)
				{
					InitilizePhysicsJoints2D({ entity, this });

				}
			}
			
		}

		{
			auto view = m_Registry.view<ScriptComponent>();

			for (auto entity : view)
			{
				InitilizeEntitiyScript({ entity, this });
			}
		}

		UpdateTransforms();
	}

	void Scene::Render2D()
	{
		auto command = Application::GetRenderPlatform()->GetRenderCommand();
		auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
		auto animatedView = m_Registry.view<TransformComponent, AnimatedSpriteRendererComponent>();
		auto colorView = m_Registry.view<TransformComponent, ColoredSpriteRendererComponent>();
		auto scriptView = m_Registry.view<ScriptComponent>();
		auto lineView = m_Registry.view<LineRendererComponent>();
		command->Clear();

		for (auto it : SortingLayer2DHandler::GetRegisteredLayers())
		{
			for (auto entity : view)
			{
				auto& spriteRenderer = view.get<SpriteRendererComponent>(entity);

				if (spriteRenderer.sprite.GetSortingLayer() == it.name)
				{
					auto& transform = view.get<TransformComponent>(entity);

					Renderer2D::DrawSprite(spriteRenderer.sprite, transform.GetTransformMatrix());
				}

			}

			for (auto entity : animatedView)
			{
				auto& animatedSprite = animatedView.get<AnimatedSpriteRendererComponent>(entity);
				auto& transform = animatedView.get<TransformComponent>(entity);

				auto dt = Application::GetTimeManager()->GetDeltaTime();
				auto frame = animatedSprite.sprite.GetFrame(dt);

				Renderer2D::DrawAnimatedSprite(animatedSprite.sprite, frame, transform.GetTransformMatrix());

			}

			if (colorView)
			{
				for (auto entity : colorView)
				{
					if (colorView.contains(entity))
					{
						auto& color = colorView.get<ColoredSpriteRendererComponent>(entity);
						auto& transform = colorView.get<TransformComponent>(entity);
						//Renderer2D::DrawQuad(color.color, transform.GetTransformMatrix());
						Renderer2D::DrawColoredQuadInstanced(color.color, transform.GetTransformMatrix());
					}
				}
			}


			for (auto entity : scriptView)
			{
				auto& script = scriptView.get<ScriptComponent>(entity);
				if (script.Instance)
				{
					script.Instance->OnRender2D(it.name);
				}
			}

		}

		for (auto entity : lineView)
		{
			auto& lineComponent = lineView.get<LineRendererComponent>(entity);

			if (lineComponent.isActive)
			{
				for (auto& line : lineComponent.lines)
				{
					Renderer2D::DrawLine({ line.x, line.y }, { line.z, line.w }, lineComponent.color);
				}
			}

		}

		if (Renderer2D::GetPhysicsDebugDrawState())
		{
			m_PhysicsWorld2D.SetDebugDraw(&m_PhysicsDebugDraw2D);
			m_PhysicsDebugDraw2D.SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit);
			m_PhysicsWorld2D.m_World->DebugDraw();
		}

	}

	void Scene::RenderPickingBuffer2D()
	{
		auto command = Application::GetRenderPlatform()->GetRenderCommand();
		m_PickingBuffer->Bind();
		command->Clear();

		{
			auto view = m_Registry.view<TransformComponent, SpriteRendererComponent>();
			auto animview = m_Registry.view<TransformComponent, AnimatedSpriteRendererComponent>();

			for (auto it : SortingLayer2DHandler::GetRegisteredLayers())
			{
				for (auto entity : view)
				{
					auto& spriteRenderer = view.get<SpriteRendererComponent>(entity);

					if (spriteRenderer.sprite.GetSortingLayer() == it.name)
					{
						auto& transform = view.get<TransformComponent>(entity);

						uint32_t entityID = (uint32_t)entity;

						entityID += 1;

						glm::vec3 color;
						color.r = entityID;
						Renderer2D::DrawColoredQuadInstanced(color, transform.GetTransformMatrix());
					}

				}

				for (auto entity : animview)
				{
					auto& spriteRenderer = animview.get<AnimatedSpriteRendererComponent>(entity);

					if (spriteRenderer.sprite.GetSortingLayer() == it.name)
					{
						auto& transform = animview.get<TransformComponent>(entity);

						uint32_t entityID = (uint32_t)entity;

						entityID += 1;

						glm::vec3 color;
						color.r = entityID;
						Renderer2D::DrawColoredQuadInstanced(color, transform.GetTransformMatrix());
					}

				}
			}
		}

		// GUI picking
		{
			RenderGUI(true);
		}


		m_PickingBuffer->Unbind();
		
	}

	void Scene::UpdateGUIPositions()
	{
		Entity activeContainerEntity = GetGuiContainer();
		if (activeContainerEntity.IsValid())
		{
			auto& activeContainer = activeContainerEntity.GetComponent<GUIContainerComponent>();
			activeContainer.container.SetScreenSize(m_ViewportSize);
			auto view = m_Registry.view<RelationShipComponent, RectTransformComponent>();

			for (auto entity : view)
			{
				auto& relation_ship = view.get<RelationShipComponent>(entity);
				auto& rect_transform = view.get<RectTransformComponent>(entity);

				if (relation_ship.parent.IsValid())
				{
					glm::vec2 parent_size = {};
					glm::vec2 parent_position = {};

					if (relation_ship.parent.HasComponent<GUIContainerComponent>())
					{
						auto& container = relation_ship.parent.GetComponent<GUIContainerComponent>();
						
						Graphics::Rect parent;
						parent.SetBounds({ 0,0 }, container.container.GetScreenSize());
						rect_transform.rect.SetParent(parent);

					}

					else if (relation_ship.parent.HasComponent<RectTransformComponent>())
					{
						auto& parent_rect = relation_ship.parent.GetComponent<RectTransformComponent>();

						rect_transform.rect.SetParent(parent_rect.GetRect());

						if (relation_ship.prev.IsValid())
						{
							if (relation_ship.prev.HasComponent<RectTransformComponent>())
							{
								auto& prev_rect = relation_ship.prev.GetComponent<RectTransformComponent>();
								rect_transform.rect.SetPreviousChild(prev_rect.GetRect());
								auto& prev_tag = relation_ship.prev.GetComponent<TagComponent>();
								Entity e{ entity, this };
								auto& tag = e.GetComponent<TagComponent>();
							}
						}
					}

					
				}

				rect_transform.rect.RecalculateRect();
			}
		}

	}

	void Scene::RenderGUIElement(Entity parent, bool pickingPhase)
	{
		Entity activeContainerEntity = GetGuiContainer();
		auto& activeContainer = activeContainerEntity.GetComponent<GUIContainerComponent>();

		if (parent.IsValid())
		{
			auto& parent_relation = parent.GetComponent<RelationShipComponent>();

			Entity current_child = parent_relation.first_child;

			for (size_t i = 0; i < parent_relation.children; i++)
			{
				if (current_child.IsValid())
				{
					auto& current_child_relation = current_child.GetComponent<RelationShipComponent>();
					size_t child_id = (size_t)current_child.m_Handle;

					// Draw gui components here
					if (current_child.HasComponent<RectTransformComponent>())
					{
						auto& rect_transform = current_child.GetComponent<RectTransformComponent>();
						if (parent.HasComponent<RectTransformComponent>())
						{
							auto& parent_rect = parent.GetComponent<RectTransformComponent>();
							rect_transform.rect.SetParent(parent_rect.rect.m_Rect);
						}
						if (Renderer2D::GetGUIDebugDrawState())
						{
							if (!pickingPhase)
							{
								Renderer2D::DrawRect(rect_transform.GetRect(), { 1,0,0 }, false, activeContainer.container.GetProjection());
							}
						}

						if (current_child.HasComponent<GUITextComponent>())
						{
							auto& guitext = current_child.GetComponent<GUITextComponent>();
							if (!guitext.fontAssetID.empty())
							{
								auto fontdesc = Application::GetAssetManager()->GetDescriptorByID(guitext.fontAssetID);

								if (fontdesc.absolutePath != guitext.uitext.m_FontFilePath)
								{
									guitext.uitext.SetFont(fontdesc.absolutePath);
								}
							}
							if (guitext.uitext.IsValid())
							{
								guitext.uitext.SetBoundingBox(rect_transform.rect);
								guitext.uitext.SetText(guitext.text);


								if (guitext.uitext.GetColor() != guitext.color)
								{
									guitext.uitext.SetColor(guitext.color);
								}

								if (guitext.fontSize != guitext.uitext.GetOriginalFontSize())
								{
									guitext.uitext.SetOriginalFontSize(guitext.fontSize);
								}

								if (guitext.alignment != guitext.uitext.GetAlignment())
								{
									guitext.uitext.SetAlignment(guitext.alignment);
								}

								if (guitext.fittingMode != guitext.uitext.GetFittingMode())
								{
									guitext.uitext.SetFittingMode(guitext.fittingMode);
								}

								if (!pickingPhase)
								{
									Renderer2D::RenderText(guitext.uitext, activeContainer.container.GetProjection());
								}
							}
							
						}

						if (current_child.HasComponent<GUITextInputComponent>())
						{
							auto& textinput = current_child.GetComponent<GUITextInputComponent>();
							if (textinput.textinput.GetTextInputRect() != rect_transform.rect)
							{
								textinput.textinput.SetTextInputRect(rect_transform.rect);
							}
							
							if (!textinput.fontAssetID.empty())
							{
								auto fontdesc = Application::GetAssetManager()->GetDescriptorByID(textinput.fontAssetID);

								if (fontdesc.absolutePath != textinput.textinput.GetUIText().m_FontFilePath)
								{
									textinput.textinput.GetUIText().SetFont(fontdesc.absolutePath);
								}
							}

							if (!pickingPhase)
							{
								Renderer2D::DrawRect(textinput.textinput.GetTextInputRect().GetRect(), textinput.textinput.GetTextInputColor(), true, activeContainer.container.GetProjection());
								Renderer2D::RenderText(textinput.textinput.GetUIText(), activeContainer.container.GetProjection());
							}
							if (pickingPhase)
							{
								child_id += 1;
								glm::vec3 color;
								color.r = child_id;
								Renderer2D::DrawRect(textinput.textinput.GetTextInputRect().GetRect(), color, true, activeContainer.container.GetProjection());
							}
						}

						if (current_child.HasComponent<GUIButtonComponent>())
						{
							auto& guibutton = current_child.GetComponent<GUIButtonComponent>();
							guibutton.button.SetUIRect(rect_transform.rect);
							if (pickingPhase)
							{
								child_id += 1;
								glm::vec3 color;
								color.r = child_id;
								Renderer2D::DrawRect(guibutton.button.GetUIRect().GetRect(), color, true, activeContainer.container.GetProjection());
							}
							else
							{
								Renderer2D::DrawRect(guibutton.button.GetUIRect().GetRect(), guibutton.button.GetColor(), true, activeContainer.container.GetProjection());
							}
						}

						if (current_child.HasComponent<GUIPanelComponent>())
						{
							auto& guipanel = current_child.GetComponent<GUIPanelComponent>();
							guipanel.panel.SetUIRect(rect_transform.rect);
							if (!pickingPhase && !guipanel.panel.IsTransparent())
							{
								Renderer2D::DrawRect(guipanel.panel.GetUIRect().GetRect(), guipanel.panel.GetColor(), true, activeContainer.container.GetProjection());
							}
						}

						if (current_child.HasComponent<GUICheckBoxComponent>())
						{
							auto& checkbox = current_child.GetComponent<GUICheckBoxComponent>();
							checkbox.box.SetBoxUIRect(rect_transform.rect);

							if (!pickingPhase)
							{
								Renderer2D::DrawRect(checkbox.box.GetBoxUIRect().GetRect(), checkbox.box.GetBoxColor(), true, activeContainer.container.GetProjection());
								if (checkbox.box.IsChecked())
								{
									Renderer2D::DrawRect(checkbox.box.GetMarkUIRect().GetRect(), checkbox.box.GetMarkColor(), true, activeContainer.container.GetProjection());
								}

							}
							if (pickingPhase)
							{
								child_id += 1;
								glm::vec3 color;
								color.r = child_id;
								Renderer2D::DrawRect(checkbox.box.GetBoxUIRect().GetRect(), color, true, activeContainer.container.GetProjection());
							}
						}

						if (current_child.HasComponent<GUISliderComponent>())
						{
							auto& slider = current_child.GetComponent<GUISliderComponent>();
							slider.slider.SetSliderRect(rect_transform.rect);

							if (!pickingPhase)
							{
								Renderer2D::DrawRect(slider.slider.GetSliderRect().GetRect(), slider.slider.GetSliderColor(), true, activeContainer.container.GetProjection());
								Renderer2D::DrawRect(slider.slider.GetKnobRect().GetRect(), slider.slider.GetKnobColor(), true, activeContainer.container.GetProjection());
							}
							if (pickingPhase)
							{
								child_id += 1;
								glm::vec3 color;
								color.r = child_id;
								Renderer2D::DrawRect(slider.slider.GetSliderRect().GetRect(), color, true, activeContainer.container.GetProjection());
							}
						}
					}

					RenderGUIElement(current_child, pickingPhase); // draw the child elements of the current child
					current_child = current_child_relation.next;
				}
			}
		}
	}

	void Scene::RenderGUI(bool pickingPhase)
	{
		UpdateGUIPositions();

		Entity activeContainerEntity = GetGuiContainer();
		if (activeContainerEntity.IsValid())
		{
			auto& activeContainer = activeContainerEntity.GetComponent<GUIContainerComponent>();

			RenderGUIElement(activeContainerEntity, pickingPhase);
		}
		
	}	

	void Scene::BeginRenderer2D(float aspectRatio)
	{
		auto view = m_Registry.view<TransformComponent, CameraComponent>();
		bool foundCamera = false;

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& camera = view.get<CameraComponent>(entity);

			if (camera.isActive)
			{
				camera.camera.SetAspectRatio(aspectRatio);
				Renderer2D::BeginScene(camera.camera, transform.GetTransformMatrix());
				break;
			}
		}
	}

	void Scene::Update()
	{

		// Update physics
		{
			m_PhysicsWorld2D.SetContactListener(&m_PhysicsListener2D);
			//m_PhysicsWorld2D.Step();

			auto view = m_Registry.view<TransformComponent, RigidBody2dComponent>();

			for (auto entity : view)
			{
				auto& transform = view.get<TransformComponent>(entity);
				auto& rigidbody2dcomponent = view.get<RigidBody2dComponent>(entity);

				if (rigidbody2dcomponent.body.IsValid())
				{
					glm::vec2 position = rigidbody2dcomponent.body.GetPosition();
					float rotation = rigidbody2dcomponent.body.GetRotation();
					transform.SetPostion({ position.x, position.y, 0.0f });
					transform.SetRotation({ 0, 0, rotation });
				}


			}
		}

		UpdateTransforms();

		CleanUpDestroyedEntities();

		// Handle GUI mouse events
		{
			auto input = Application::GetInputManager();

			if (input->GetMouseDown(MouseButtons::LEFT))
			{
				int mouseX = input->GetMouseX();
				int mouseY = input->GetMouseY();

				if (mouseX < m_ViewportRect.GetMax().x && mouseY < m_ViewportRect.GetMax().y)
				{
					if (mouseX > m_ViewportRect.GetMin().x && mouseY > m_ViewportRect.GetMin().y)
					{
						int bufferX = mouseX - (int)m_ViewportRect.GetMin().x;
						int bufferY = mouseY - (int)m_ViewportRect.GetMin().y;
						auto pixel = m_PickingBuffer->ReadPixels(bufferX, m_ViewportSize.y - bufferY - 1);
						
						unsigned int entityID = pixel.x;

						entityID -= 1;
						Entity PickedEntity = Entity((entt::entity)entityID, this);
						if (PickedEntity.IsValid())
						{
							m_LastPickedEntity = (entt::entity) - 1;

							if (PickedEntity.HasComponent<GUIButtonComponent>())
							{
								auto& uibutton = PickedEntity.GetComponent<GUIButtonComponent>();

								if (uibutton.button.m_Callback)
								{
									uibutton.button.m_Callback();
								}
								m_LastPickedEntity = PickedEntity.m_Handle;
							}

							if (PickedEntity.HasComponent<GUICheckBoxComponent>())
							{
								auto& checkBox = PickedEntity.GetComponent<GUICheckBoxComponent>();
								checkBox.box.SetCheckStatus(!checkBox.box.IsChecked());
							}

							if (PickedEntity.HasComponent<GUITextInputComponent>())
							{
								m_LastPickedEntity = PickedEntity.m_Handle;
							}
						}
						else
						{
							m_LastPickedEntity = (entt::entity)-1;
						}
	
					}
				}
			}

			if (input->IsMouseDown(MouseButtons::LEFT))
			{
				int mouseX = input->GetMouseX();
				int mouseY = input->GetMouseY();

				if (mouseX < m_ViewportRect.GetMax().x && mouseY < m_ViewportRect.GetMax().y)
				{
					if (mouseX > m_ViewportRect.GetMin().x && mouseY > m_ViewportRect.GetMin().y)
					{
						int bufferX = mouseX - (int)m_ViewportRect.GetMin().x;
						int bufferY = mouseY - (int)m_ViewportRect.GetMin().y;
						auto pixel = m_PickingBuffer->ReadPixels(bufferX, m_ViewportSize.y - bufferY - 1);

						unsigned int entityID = pixel.x;

						entityID -= 1;
						Entity PickedEntity = Entity((entt::entity)entityID, this);
						if (PickedEntity.IsValid())
						{
							if (PickedEntity.HasComponent<GUISliderComponent>())
							{
								m_LastPickedEntity = (entt::entity)entityID;
								auto& slider = PickedEntity.GetComponent<GUISliderComponent>();
								glm::vec2 sliderMin = slider.slider.GetSliderRect().GetRect().GetMin();
								glm::vec2 sliderMax = slider.slider.GetSliderRect().GetRect().GetMax();
								if (bufferX < sliderMax.x && bufferY < sliderMax.y)
								{
									if (bufferX > sliderMin.x && bufferY > sliderMin.y)
									{
										float sliderX = bufferX - slider.slider.GetSliderRect().GetRect().GetMin().x;
										if (sliderX < sliderMax.x)
										{
											slider.slider.SetKnobX(sliderX);
										}
									}
								}
							}
						}

					}
				}
			}

			Entity lastEntity = { m_LastPickedEntity, this };
			if (lastEntity.IsValid())
			{
				if (lastEntity.HasComponent<GUITextInputComponent>())
				{
					auto character = input->GetCharacterDown();
					if (character >= 0 && character < 128)
					{
						char c = character;
						auto& textInput = lastEntity.GetComponent<GUITextInputComponent>();
						if (c == 8)
						{
							textInput.textinput.RemoveCharacter();
						}
						else
						{
							textInput.textinput.AddCharacter(c);
						}
					}
				}
			}
		}

		// Update scripts
		{
		auto view = m_Registry.view<ScriptComponent>();

		for (auto entity : view)
		{
			auto& script = view.get<ScriptComponent>(entity);
			if (script.Instance != nullptr)
			{
				try
				{
					script.Instance->OnUpdate();
				}
				catch (const std::exception& e)
				{
					AK_ERROR(e.what());
				}

			}

		}
		}
	}

	void Scene::Stop()
	{

		{
			m_PhysicsWorld2D.Clear();
			auto view = m_Registry.view<RigidBody2dComponent>();
			for (auto entity : view)
			{
				auto& rigidbody2dcomponent = view.get<RigidBody2dComponent>(entity);

				rigidbody2dcomponent.body = Box2dBody();
			}
		}

		{
			auto view = m_Registry.view<ScriptComponent>();

			for (auto entity : view)
			{
				auto& script = view.get<ScriptComponent>(entity);

				if (script.Instance != nullptr)
				{
					delete script.Instance;
					script.Instance = nullptr;
				}

			}
		}
		
	}

	void Scene::UpdateTransforms()
	{
		auto view = m_Registry.view<TransformComponent, RelationShipComponent>();
		if (view)
		{
			for (auto entity : view)
			{
				Entity e = { entity, this };
				if (!e.HasComponent<ColoredSpriteRendererComponent>())
				{
					if (view.contains(entity))
					{
						auto& relation_ship = view.get<RelationShipComponent>(entity);
						auto& child_transform = view.get<TransformComponent>(entity);

						if (relation_ship.parent.IsValid())
						{
							if (relation_ship.parent.HasComponent<TransformComponent>())
							{
								auto& parent_transform = relation_ship.parent.GetComponent<TransformComponent>();
								//child_transform.m_ParentPosition = parent_transform.GetPosition();
								//child_transform.m_ParentRotation = parent_transform.GetRotation();
								child_transform.SetParentPosition(parent_transform.GetPosition());
								child_transform.SetParentRotation(parent_transform.GetRotation());
							}
						}
					}
				}

				
			}
		}

	}

	void Scene::InitilizePhysicsBodies2D(Entity entity)
	{
		if (entity.HasComponent<RigidBody2dComponent>())
		{
			auto& rigidbody2dcomp = entity.GetComponent<RigidBody2dComponent>();
			auto& transform = entity.GetComponent<TransformComponent>();

			BodySettings settings;
			settings.density = rigidbody2dcomp.density;
			settings.friction = rigidbody2dcomp.friction;
			settings.shape = rigidbody2dcomp.shape;
			settings.type = rigidbody2dcomp.type;

			settings.position = { transform.GetPosition().x, transform.GetPosition().y };
			settings.rotation = { transform.GetRotation().z };

			settings.halfX = transform.GetScale().x / 2;
			settings.halfY = transform.GetScale().y / 2;

			rigidbody2dcomp.body = m_PhysicsWorld2D.CreateBody(settings, this, (uint32_t)entity.m_Handle);
		}
	}

	void Scene::InitilizePhysicsJoints2D(Entity entity)
	{
		if (entity.HasComponent<HingeJoint2DComponent>())
		{
			auto& hinge = entity.GetComponent<HingeJoint2DComponent>();

			if (hinge.bodyA.IsValid() && hinge.bodyB.IsValid())
			{
				if (hinge.bodyA.HasComponent<RigidBody2dComponent>() && hinge.bodyA.HasComponent<RigidBody2dComponent>())
				{
					b2RevoluteJointDef def;
					def.bodyA = hinge.bodyA.GetComponent<RigidBody2dComponent>().GetBody();
					def.bodyB = hinge.bodyB.GetComponent<RigidBody2dComponent>().GetBody();

					def.localAnchorA = { hinge.localAnchorA.x, hinge.localAnchorA.y };
					def.localAnchorB = { hinge.localAnchorB.x, hinge.localAnchorB.y };

					def.collideConnected = hinge.collideConnected;

					def.enableMotor = hinge.enableMotor;
					def.motorSpeed = hinge.motorSpeed;
					def.maxMotorTorque = hinge.maxMotorTorque;

					hinge.joint = (b2RevoluteJoint*)m_PhysicsWorld2D.CreateJoint(&def);
				}
			}
		}
	}

	void Scene::InitilizeEntitiyScript(Entity entity)
	{
		if (entity.HasComponent<ScriptComponent>())
		{
			auto& script = entity.GetComponent<ScriptComponent>();

			if (script.Instance == nullptr)
			{
				//TODO : REPLACE GAME ASSEMBLY SYSTEM WITH A STATIC ONE
				script.Instance = nullptr;
				script.Instance->m_Entity = entity;

				try
				{
					script.Instance->OnStart();
				}
				catch (const std::exception& e)
				{
					AK_ERROR(e.what());
				}

			}
		}
	}

	void Scene::SetViewportSize(glm::vec2 size)
	{
		m_PickingBuffer->SetSize(size.x, size.y);
		m_ViewportSize = size;
	}

	Entity Scene::AddEntity(std::string tag)
	{
		Entity entity = { m_Registry.create(), this };

		// any entity must have these components by default
		auto& tag_comp = entity.AddComponent<TagComponent>();
		auto& transform_comp = entity.AddComponent<TransformComponent>();
		auto& relation_ship = entity.AddComponent<RelationShipComponent>();

		relation_ship.children = 0;
		tag_comp.Tag = tag;

		return entity;
	}

	Entity Scene::AddEntity(uint32_t hint, std::string tag)
	{
		Entity entity = { m_Registry.create((entt::entity)hint), this };

		// any entity must have these components by default
		auto& tag_comp = entity.AddComponent<TagComponent>();
		auto& transform_comp = entity.AddComponent<TransformComponent>();
		auto& relation_ship = entity.AddComponent<RelationShipComponent>();

		relation_ship.children = 0;
		tag_comp.Tag = tag;

		return entity;
	}

	Entity Scene::GetEntityByTag(std::string tag)
	{
		auto view = m_Registry.view<TagComponent>();

		for (auto entity : view)
		{
			if (view.contains(entity))
			{
				auto& tagComponent = view.get<TagComponent>(entity);

				if (tagComponent.Tag == tag)
				{
					Entity found = { entity, this };
					return found;
				}
			}
			else
			{
				AK_ERROR("Could not get entity by tag : {}, view does not contain the entity !", tag);
				return Entity();
			}

		}
		AK_ERROR("Could not get entity by tag : {}", tag);
		// return an invalid entity.
		return Entity();
	}

	void Scene::AssignEntityToParent(Entity parent, Entity child)
	{
		auto& child_relation = child.GetComponent<RelationShipComponent>();

		if (!EntityHasChild(parent, child) && !EntityHasHierarchyChild(child, parent))
		{
			if (child_relation.parent.IsValid())
			{
				auto& old_parent_relation = child_relation.parent.GetComponent<RelationShipComponent>();
				if (old_parent_relation.first_child == child)
				{
					if (child_relation.next.IsValid())
					{
						old_parent_relation.first_child = child_relation.next;
					}
					else
					{
						old_parent_relation.first_child = {};
					}
				}

				if (old_parent_relation.last_child == child)
				{
					if (child_relation.prev.IsValid())
					{
						old_parent_relation.last_child = child_relation.prev;
					}
				}
				old_parent_relation.children -= 1;
			}

			if (child_relation.next.IsValid())
			{
				auto& next_relation = child_relation.next.GetComponent<RelationShipComponent>();
				next_relation.prev = child_relation.prev;
			}

			if (child_relation.prev.IsValid())
			{
				auto& prev_relation = child_relation.prev.GetComponent<RelationShipComponent>();
				prev_relation.next = child_relation.next;
			}

			if (parent.IsValid())
			{
				auto& parent_relation = parent.GetComponent<RelationShipComponent>();

				if (parent_relation.children == 0)
				{
					parent_relation.first_child = child;
					parent_relation.last_child = child;
				}

				else
				{
					Entity last_child = parent_relation.last_child;
					if (last_child.IsValid())
					{
						auto& last_child_relation = last_child.GetComponent<RelationShipComponent>();
						child_relation.prev = last_child;
						last_child_relation.next = child;
					}
				}

				parent_relation.last_child = child;
				parent_relation.children += 1;
			}

			child_relation.parent = parent;


		}
	}

	bool Scene::EntityHasChild(Entity parent, Entity child)
	{
		if (parent.IsValid())
		{
			const auto& parent_relation = parent.GetComponent<RelationShipComponent>();

			Entity current_child = parent_relation.first_child;

			for (size_t i = 0; i < parent_relation.children; i++)
			{
				if (current_child == child)
				{
					return true;
				}

				auto& current_child_relation = current_child.GetComponent<RelationShipComponent>();

				current_child = current_child_relation.next;
			}

			return false;
		}
		
	}

	bool Scene::EntityHasHierarchyChild(Entity parent, Entity child)
	{
		auto& parent_relation = parent.GetComponent<RelationShipComponent>();

		Entity current_child = parent_relation.first_child;
		for (size_t i = 0; i < parent_relation.children; i++)
		{
			if (current_child.IsValid())
			{
				auto& current_child_relation = current_child.GetComponent<RelationShipComponent>();
				if (current_child == child)
				{
					return true;
				}

				else if(current_child_relation.first_child.IsValid())
				{
					if (EntityHasHierarchyChild(current_child, current_child_relation.first_child))
					{
						return true;
					}
				}

			}

		}

		return false;
	}

	Entity Scene::InstantiateEntity(std::string instantiableEntityName, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
	{
		auto entity = InstantiateEntityStatic(instantiableEntityName, position, rotation, scale);
		if (entity.IsValid())
		{
			InitilizePhysicsBodies2D(entity);
			InitilizePhysicsJoints2D(entity);
			InitilizeEntitiyScript(entity);
			return entity;
		}
		return Entity();
	}

	Entity Scene::InstantiateEntityStatic(std::string instantiableEntityName, glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
	{
		auto assetManager = Application::GetAssetManager();
		auto desc = assetManager->GetAssetByName(instantiableEntityName);
		if (desc.assetType == AssetType::INSTANTIABLE_ENTITY)
		{
			auto entity = AddEntity();
			nlohmann::ordered_json instantiable_data = *assetManager->GetInstantiableEntityByName(instantiableEntityName);

			auto entity_data = instantiable_data["Scene"]["Entities"].items().begin();
			SceneSerializer::DeserializeEntity(entity, entity_data.key(), this, instantiable_data);

			auto& transform = entity.GetComponent<TransformComponent>();
			transform.SetPostion(position);
			transform.SetRotation(rotation);
			transform.SetScale(scale);
			return entity;
		}
		else
		{
			AK_ERROR("Could not instantiate entity : {} maybe the entity file was deleted or it doesen't exist !", instantiableEntityName);
			return Entity();
		}
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntitiesToDestroy.push_back(entity.m_Handle);
	}

	void Scene::RemoveEntity(Entity entity)
	{
		auto& entity_relation = entity.GetComponent<RelationShipComponent>();

		Entity current_child = entity_relation.first_child;
		for (size_t i = 0; i < entity_relation.children; i++)
		{
			if (current_child.IsValid())
			{
				auto& current_child_relation = current_child.GetComponent<RelationShipComponent>();
				current_child = current_child_relation.next;

				current_child_relation.parent = {};
				current_child_relation.next = {};
				current_child_relation.prev = {};

			}
		}

		if (entity_relation.parent.IsValid())
		{
			auto& parent_relation = entity_relation.parent.GetComponent<RelationShipComponent>();
			if (parent_relation.first_child == entity)
			{
				parent_relation.first_child = entity_relation.next;
			}

			else if (parent_relation.last_child == entity)
			{
				parent_relation.last_child = entity_relation.prev;
			}

			parent_relation.children -= 1;
		}

		if (entity_relation.next.IsValid())
		{
			auto& next_relation = entity_relation.next.GetComponent<RelationShipComponent>();
			next_relation.prev = entity_relation.prev;
		}

		if (entity_relation.prev.IsValid())
		{
			auto& prev_relation = entity_relation.prev.GetComponent<RelationShipComponent>();
			prev_relation.next = entity_relation.next;
		}

		m_Registry.destroy(entity.m_Handle);
	}

	void Scene::RemoveEntityWithAllChildren(Entity entity)
	{
		auto& entity_relation = entity.GetComponent<RelationShipComponent>();

		Entity current_child = entity_relation.last_child;
		for (size_t i = 0; i < entity_relation.children; i++)
		{
			if (current_child.IsValid())
			{
				auto& current_child_relation = current_child.GetComponent<RelationShipComponent>();
				Entity cache = current_child;
				current_child = current_child_relation.prev;

				m_Registry.destroy(cache.m_Handle);

			}
		}

		if (entity_relation.parent.IsValid())
		{
			auto& parent_relation = entity_relation.parent.GetComponent<RelationShipComponent>();
			if (parent_relation.first_child == entity)
			{
				parent_relation.first_child = entity_relation.next;
			}

			else if (parent_relation.last_child == entity)
			{
				parent_relation.last_child = entity_relation.prev;
			}

			parent_relation.children -= 1;
		}

		if (entity_relation.next.IsValid())
		{
			auto& next_relation = entity_relation.next.GetComponent<RelationShipComponent>();
			next_relation.prev = entity_relation.prev;
		}

		if (entity_relation.prev.IsValid())
		{
			auto& prev_relation = entity_relation.prev.GetComponent<RelationShipComponent>();
			prev_relation.next = entity_relation.next;
		}

		m_Registry.destroy(entity.m_Handle);
	}

	Entity Scene::GetGuiContainer()
	{
		auto containerView = m_Registry.view<GUIContainerComponent>();

		for (auto entity : containerView)
		{
			return Entity(entity, this);
		}

		return Entity(); // return an invalid entity if the scene doesn't have a gui container
	}

	Entity Scene::AddGuiContainer()
	{
		Entity guicontainer = Entity(m_Registry.create(), this);
		guicontainer.AddComponent<RelationShipComponent>();
		guicontainer.AddComponent<GUIContainerComponent>();
		auto& tag = guicontainer.AddComponent<TagComponent>();
		tag.Tag = "gui container";

		return guicontainer;
		
	}

	void Scene::CleanUpDestroyedEntities()
	{
		for (auto e : m_EntitiesToDestroy)
		{
			Entity entity = { e, this };
			if (entity.HasComponent<RigidBody2dComponent>())
			{
				auto& rb = entity.GetComponent<RigidBody2dComponent>();
				m_PhysicsWorld2D.m_World->DestroyBody(rb.GetBody());
				rb.body = Box2dBody();
			}

			if (entity.HasComponent<ScriptComponent>())
			{
				auto& script = entity.GetComponent<ScriptComponent>();

				if (script.Instance != nullptr)
				{
					delete script.Instance;
					script.Instance = nullptr;
				}
			}

			RemoveEntity(entity);
			m_EntitiesToDestroy.erase(std::remove(m_EntitiesToDestroy.begin(), m_EntitiesToDestroy.end(), e), m_EntitiesToDestroy.end());
		}
	}

	Entity Scene::GetActiveCamera()
	{
		auto view = m_Registry.view<TransformComponent, CameraComponent>();
		bool foundCamera = false;

		for (auto entity : view)
		{
			auto& transform = view.get<TransformComponent>(entity);
			auto& camera = view.get<CameraComponent>(entity);

			if (camera.isActive)
			{
				return {entity, this};
			}
		}
	}

	Entity Scene::GetEntity(entt::entity handle)
	{
		Entity entity = { handle, this };
		if (entity.IsValid())
		{
			return entity;
		}
		else
		{
			AK_ASSERT(false, "Invalid Entity");
		}
	}

}

