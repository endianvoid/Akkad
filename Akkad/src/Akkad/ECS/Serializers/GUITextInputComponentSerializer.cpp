#include "GUITextInputComponentSerializer.h"
#include "Akkad/ECS/Components/GUITextInputComponent.h"
namespace Akkad {

	void GUITextInputComponentSerializer::Serialize(Entity entity, json& entity_data)
	{
		auto& textinput = entity.GetComponent<GUITextInputComponent>();

		entity_data["GUITextInputComponent"]["TextInputColor"][0] = textinput.textinput.GetTextInputColor().r;
		entity_data["GUITextInputComponent"]["TextInputColor"][1] = textinput.textinput.GetTextInputColor().g;
		entity_data["GUITextInputComponent"]["TextInputColor"][2] = textinput.textinput.GetTextInputColor().b;

		entity_data["GUITextInputComponent"]["TextColor"][0] = textinput.textinput.GetTextColor().r;
		entity_data["GUITextInputComponent"]["TextColor"][1] = textinput.textinput.GetTextColor().g;
		entity_data["GUITextInputComponent"]["TextColor"][2] = textinput.textinput.GetTextColor().b;

		entity_data["GUITextInputComponent"]["FontAssetID"] = textinput.fontAssetID;
	}

	void GUITextInputComponentSerializer::Deserialize(Entity entity, json& component_data)
	{
		glm::vec3 textInputColor;
		textInputColor.r = component_data["TextInputColor"][0];
		textInputColor.g = component_data["TextInputColor"][1];
		textInputColor.b = component_data["TextInputColor"][2];

		glm::vec3 textColor;
		textColor.r = component_data["TextColor"][0];
		textColor.g = component_data["TextColor"][1];
		textColor.b = component_data["TextColor"][2];

		std::string fontAssetID = component_data["FontAssetID"];

		auto& textInput = entity.AddComponent<GUITextInputComponent>();

		textInput.textinput.SetTextInputColor(textInputColor);
		textInput.textinput.SetTextColor(textColor);
		textInput.fontAssetID = fontAssetID;
	}
}