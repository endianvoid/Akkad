#include "GLFrameBuffer.h"
#include "GLTexture.h"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

namespace Akkad {
	namespace Graphics {

		GLFrameBuffer::GLFrameBuffer(FrameBufferDescriptor desc)
		{
			m_desc = desc;
			glGenFramebuffers(1, &m_ResourceID);
			Bind();
			if (desc.hasColorAttachment)
			{
				if (desc.usesRenderBuffer)
				{
					glGenRenderbuffers(1, &m_ColorAttachmentTextureID);
				}
				else
				{
					glGenTextures(1, &m_ColorAttachmentTextureID);
				}
			}
			UpdateTexture();
			Unbind();
		}

		GLFrameBuffer::~GLFrameBuffer()
		{
		}

		void GLFrameBuffer::UpdateTexture()
		{
			if (m_desc.hasColorAttachment)
			{
				Bind();
				if (m_desc.usesRenderBuffer)
				{
					glBindRenderbuffer(GL_RENDERBUFFER, m_ColorAttachmentTextureID);
					glRenderbufferStorage(GL_RENDERBUFFER, GLTexture::TextureFormatToGLFormat(m_desc.ColorAttachmentFormat), m_desc.width, m_desc.height);
					glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_ColorAttachmentTextureID);
					glBindRenderbuffer(GL_RENDERBUFFER, 0);
				}
				else
				{
					glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentTextureID);
					
					glTexImage2D(GL_TEXTURE_2D, 0, GLTexture::TextureFormatToGLFormat(m_desc.ColorAttachmentFormat), m_desc.width, m_desc.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachmentTextureID, 0);

					glBindTexture(GL_TEXTURE_2D, 0);
				}
				Unbind();
			}

		}

		void GLFrameBuffer::Bind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_ResourceID);
			glViewport(0, 0, m_desc.width, m_desc.height);
		}

		void GLFrameBuffer::Unbind()
		{
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		void GLFrameBuffer::SetSize(unsigned int width, unsigned int height)
		{
			m_desc.width = width;
			m_desc.height = height;

			UpdateTexture();
		}

		unsigned int GLFrameBuffer::GetColorAttachmentTexture()
		{
			return m_ColorAttachmentTextureID;
		}

		glm::vec4 GLFrameBuffer::ReadPixels(int x, int y)
		{
			glm::vec4 pixelData;

			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ResourceID);

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glReadPixels(x, y, 1, 1, GL_RED, GL_FLOAT, glm::value_ptr(pixelData));

			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
			return pixelData;
		}

		FrameBufferDescriptor& GLFrameBuffer::GetDescriptor()
		{
			return m_desc;
		}

	}
}


