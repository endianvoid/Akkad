#pragma once

#include <string>

namespace Akkad {

	class GameAssemblyHandler
	{
	public:
		static void CompileGameAssembly(std::string path);

		static void LoadGameAssembly(std::string filename);
		static void FreeGameAssembly();
		static bool loadedGameAssembly;
	private:
	};
}

