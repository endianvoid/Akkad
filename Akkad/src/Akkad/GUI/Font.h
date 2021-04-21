#pragma once
#include "Akkad/core.h"

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <map>

namespace Akkad {

	// forward declarations
	namespace Graphics {
		class Texture;
	}

	namespace GUI {

		class Font {

		public:
			struct FontCharacter {
				glm::ivec2   Size;       // Size of glyph
				glm::ivec2   Bearing;    // Offset from baseline to left/top of glyph
				unsigned int Advance;    // Offset to advance to next glyph
				float xOffset;			 // The X offset of the glyph from the texture atlas
			};

			Font();
			Font(std::string filePath);
			Font(std::string filePath, unsigned int pixelSize);
			
			bool SetFontFile(std::string filePath, unsigned int pixelSize);
			unsigned int GetTextureAtlasID();
			glm::vec2 GetTextureAtlasSize();
			SharedPtr<Graphics::Texture> GetAtlas() { return m_FontTextureAtlas; };
			FontCharacter GetCharacter(unsigned long code);
		private:


			void InitFreeType();
			glm::vec2 CalculateTextureAtlasSize();
			void GenerateTextureAtlas();

			static FT_Library s_FTLib;
			FT_Face m_Face;

			SharedPtr<Graphics::Texture> m_FontTextureAtlas;
			glm::vec2 m_TextureAtlasSize;
			std::map<unsigned long, FontCharacter> m_FontCharacters;
			
		};
	}
}