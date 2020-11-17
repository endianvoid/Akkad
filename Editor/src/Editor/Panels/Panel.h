#pragma once
#include <vector>

namespace Akkad {

	class Panel {
	public:
		virtual void DrawImGui() = 0;
		virtual void OnOpen() = 0;
		virtual void OnClose() = 0;
		virtual bool IsOpen() = 0;
	};

	class PanelManager {
	public:

		static void AddPanel(Panel* panel)
		{
			if (!panel->IsOpen())
			{
				s_Panels.push_back(panel);
				panel->OnOpen();
				
			}
		}

		static void RemovePanel(Panel* panel)
		{
			s_Panels.erase(std::remove(s_Panels.begin(), s_Panels.end(), panel), s_Panels.end());
			delete panel;
		}

		static std::vector<Panel*> GetPanels() { return s_Panels; }
	private:
		static std::vector<Panel*> s_Panels;
	};
}