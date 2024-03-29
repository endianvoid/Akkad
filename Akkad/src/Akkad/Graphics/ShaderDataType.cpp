#include "ShaderDataType.h"

namespace Akkad {
	namespace Graphics {
		unsigned int GetSizeOfType(ShaderDataType type)
		{
			switch (type)
			{
			case ShaderDataType::FLOAT:
				return 4;
			case ShaderDataType::FLOAT2:
				return 4 * 2;
			case ShaderDataType::FLOAT3:
				return 4 * 3;
			case ShaderDataType::FLOAT4:
				return 4 * 4;
			case ShaderDataType::UNISGNED_INT:
				return 4;
			case ShaderDataType::MAT3:
				return 4 * 3 * 3;
			case ShaderDataType::MAT4:
				return 4 * 4 * 4;
			case ShaderDataType::BOOL:
				return 4;

			default:
				break;
			}
		}
	}
}