#pragma once
#include "Akkad/Graphics/FrameBuffer.h"

namespace Akkad {
	namespace Graphics {

		class GLFrameBuffer : public FrameBuffer
		{
		public:
			GLFrameBuffer(FrameBufferDescriptor desc);
			~GLFrameBuffer();

			virtual void Bind() override;
			virtual void Unbind() override;
			virtual void SetSize(unsigned int width, unsigned int height) override;

			virtual unsigned int GetColorAttachmentTexture() override;
			virtual glm::vec4 ReadPixels(int x, int y) override;

			virtual FrameBufferDescriptor& GetDescriptor() override;
		private:
			void UpdateTexture();
			unsigned int m_ResourceID;
			unsigned int m_ColorAttachmentTextureID;

			FrameBufferDescriptor m_desc;
		};
	}
}


