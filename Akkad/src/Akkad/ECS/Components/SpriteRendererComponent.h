#pragma once
#include "Akkad/Graphics/Sprite.h"

#include <glm/glm.hpp>
#include <string>

namespace Akkad {

	struct SpriteRendererComponent {
		Graphics::Sprite sprite;
		std::string materialID;
	};

	struct AnimatedSpriteRendererComponent {
		Graphics::AnimatedSprite sprite;
		std::string materialID;
	};

	struct ColoredSpriteRendererComponent {
		glm::vec3 color;
	};
}