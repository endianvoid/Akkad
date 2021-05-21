#include "RectTransformComponentSerializer.h"

#include "Akkad/ECS/Components/RectTransformComponent.h"
#include "Akkad/ECS/Components/TransformComponent.h"

namespace Akkad {
	std::string ConstraintTypeToStr(GUI::ConstraintType type)
	{
		switch (type)
		{
		case GUI::ConstraintType::CENTER_CONSTRAINT:
			return "CENTER_CONSTRAINT";
		case GUI::ConstraintType::RELATIVE_CONSTRAINT:
			return "RELATIVE_CONSTRAINT";
		case GUI::ConstraintType::PIXEL_CONSTRAINT:
			return "PIXEL_CONSTRAINT";
		case GUI::ConstraintType::ASPECT_CONSTRAINT:
			return "ASPECT_CONSTRAINT";
		default:
			break;
		}
	}

	GUI::ConstraintType GetConstraintTypeFromStr(std::string type) {

		if (type == "CENTER_CONSTRAINT")
		{
			return GUI::ConstraintType::CENTER_CONSTRAINT;
		}

		if (type == "RELATIVE_CONSTRAINT")
		{
			return GUI::ConstraintType::RELATIVE_CONSTRAINT;
		}

		if (type == "PIXEL_CONSTRAINT")
		{
			return GUI::ConstraintType::PIXEL_CONSTRAINT;
		}

		if (type == "ASPECT_CONSTRAINT")
		{
			return GUI::ConstraintType::ASPECT_CONSTRAINT;
		}
	}

	void RectTransformComponentSerializer::Serialize(Entity entity, json& entity_data)
	{
		auto& rect_transform = entity.GetComponent<RectTransformComponent>();

		auto width_constraint = rect_transform.rect.GetWidthConstraint();
		auto height_constraint = rect_transform.rect.GetHeightConstraint();

		auto x_constraint = rect_transform.rect.GetXConstraint();
		auto y_constraint = rect_transform.rect.GetYConstraint();
		
		entity_data["RectTransformComponent"]["WidthConstraint"]["Type"] = ConstraintTypeToStr(width_constraint.type);
		entity_data["RectTransformComponent"]["WidthConstraint"]["Value"] = width_constraint.constraintValue;

		entity_data["RectTransformComponent"]["HeightConstraint"]["Type"] = ConstraintTypeToStr(height_constraint.type);
		entity_data["RectTransformComponent"]["HeightConstraint"]["Value"] = height_constraint.constraintValue;

		entity_data["RectTransformComponent"]["XConstraint"]["Type"] = ConstraintTypeToStr(x_constraint.type);
		entity_data["RectTransformComponent"]["XConstraint"]["Value"] = x_constraint.constraintValue;

		entity_data["RectTransformComponent"]["YConstraint"]["Type"] = ConstraintTypeToStr(y_constraint.type);
		entity_data["RectTransformComponent"]["YConstraint"]["Value"] = y_constraint.constraintValue;

		
	}

	void RectTransformComponentSerializer::Deserialize(Entity entity, json& component_data)
	{
		GUI::Constraint width_constraint;
		GUI::Constraint height_constraint;

		GUI::Constraint x_constraint;
		GUI::Constraint y_constraint;

		width_constraint.type = GetConstraintTypeFromStr(component_data["WidthConstraint"]["Type"]);
		width_constraint.constraintValue = component_data["WidthConstraint"]["Value"];

		height_constraint.type = GetConstraintTypeFromStr(component_data["HeightConstraint"]["Type"]);
		height_constraint.constraintValue = component_data["HeightConstraint"]["Value"];

		x_constraint.type = GetConstraintTypeFromStr(component_data["XConstraint"]["Type"]);
		x_constraint.constraintValue = component_data["XConstraint"]["Value"];

		y_constraint.type = GetConstraintTypeFromStr(component_data["YConstraint"]["Type"]);
		y_constraint.constraintValue = component_data["YConstraint"]["Value"];

		entity.RemoveComponent<TransformComponent>();

		auto& rect_transform = entity.AddComponent<RectTransformComponent>();

		rect_transform.rect.SetWidthConstraint(width_constraint);
		rect_transform.rect.SetHeightConstraint(height_constraint);

		rect_transform.rect.SetXConstraint(x_constraint);
		rect_transform.rect.SetYConstraint(y_constraint);

	}
}