#pragma once
#include "Box2dBody.h"
#include "Box2dContactListener.h"
#include "Box2dDraw.h"
#include "Akkad/core.h"

#include <box2d/box2d.h>
#include <glm/glm.hpp>

namespace Akkad {

	namespace Graphics {
		class Shader;
		class VertexBuffer;
		class UniformBuffer;
	}

	class Scene;

	class Box2dWorld
	{
	public:
		Box2dWorld();
		Box2dWorld(glm::vec2 gravity);
		~Box2dWorld();

		Box2dBody CreateBody(BodySettings settings, Scene* scene, uint32_t entityid);
		void SetContactListener(Box2dContactListener* listener);
		void SetDebugDraw(Box2dDraw* draw);
		void Step();
		b2Joint* CreateJoint(b2JointDef* def);
		void DestroyJoint(b2Joint* joint);

		void Clear();

	private:
		SharedPtr<b2World> m_World;
		glm::vec2 m_Gravity = {0.0f, -10.0f};

		friend class Scene;
	};
}


