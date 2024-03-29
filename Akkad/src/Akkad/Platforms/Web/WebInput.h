#pragma once
#include "Akkad/Input/Input.h"
namespace Akkad {

	class WebInput : public Input
	{
	public:
		virtual bool GetKeyDown(unsigned int key) override;
		virtual bool GetKeyUp(unsigned int key) override;

		virtual bool GetMouseDown(MouseButtons button) override;
		virtual bool GetMouseUp(MouseButtons button) override;

		virtual bool IsKeyDown(unsigned int key) override;
		virtual bool IsMouseDown(MouseButtons button) override;

		virtual int GetCharacterDown() override;


		virtual int GetMouseX() override;
		virtual int GetMouseY() override;
	};
}


