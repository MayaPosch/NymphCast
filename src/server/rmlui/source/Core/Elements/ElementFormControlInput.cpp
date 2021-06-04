/*
 * This source file is part of RmlUi, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://github.com/mikke89/RmlUi
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 * Copyright (c) 2019 The RmlUi Team, and contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "../../../Include/RmlUi/Core/Elements/ElementFormControlInput.h"
#include "../../../Include/RmlUi/Core/Event.h"
#include "InputTypeButton.h"
#include "InputTypeCheckbox.h"
#include "InputTypeRadio.h"
#include "InputTypeRange.h"
#include "InputTypeSubmit.h"
#include "InputTypeText.h"

namespace Rml {

// Constructs a new ElementFormControlInput.
ElementFormControlInput::ElementFormControlInput(const String& tag) : ElementFormControl(tag)
{
	// OnAttributeChange will be called right after this, possible with a non-default type. Thus,
	// creating the default InputTypeText here may result in it being destroyed in just a few moments.
	// Instead, we create the InputTypeText in OnAttributeChange in the case where the type attribute has not been set.
}

ElementFormControlInput::~ElementFormControlInput()
{}

// Returns a string representation of the current value of the form control.
String ElementFormControlInput::GetValue() const
{
	RMLUI_ASSERT(type);
	return type->GetValue();
}

// Sets the current value of the form control.
void ElementFormControlInput::SetValue(const String& value)
{
	SetAttribute("value", value);
}

// Returns if this value should be submitted with the form.
bool ElementFormControlInput::IsSubmitted()
{
	RMLUI_ASSERT(type);
	return type->IsSubmitted();
}

// Updates the element's underlying type.
void ElementFormControlInput::OnUpdate()
{
	RMLUI_ASSERT(type);
	type->OnUpdate();
}

// Renders the element's underlying type.
void ElementFormControlInput::OnRender()
{
	RMLUI_ASSERT(type);
	type->OnRender();
}

void ElementFormControlInput::OnResize()
{
	RMLUI_ASSERT(type);
	type->OnResize();
}

void ElementFormControlInput::OnLayout()
{
	RMLUI_ASSERT(type);
	type->OnLayout();
}

// Checks for necessary functional changes in the control as a result of changed attributes.
void ElementFormControlInput::OnAttributeChange(const ElementAttributes& changed_attributes)
{
	ElementFormControl::OnAttributeChange(changed_attributes);

	String new_type_name;

	auto it_type = changed_attributes.find("type");
	if (it_type != changed_attributes.end())
	{
		new_type_name = it_type->second.Get<String>("text");
	}

	if (!type || (!new_type_name.empty() && new_type_name != type_name))
	{
		if (new_type_name == "password")
			type = MakeUnique<InputTypeText>(this, InputTypeText::OBSCURED);
		else if (new_type_name == "radio")
			type = MakeUnique<InputTypeRadio>(this);
		else if (new_type_name == "checkbox")
			type = MakeUnique<InputTypeCheckbox>(this);
		else if (new_type_name == "range")
			type = MakeUnique<InputTypeRange>(this);
		else if (new_type_name == "submit")
			type = MakeUnique<InputTypeSubmit>(this);
		else if (new_type_name == "button")
			type = MakeUnique<InputTypeButton>(this);
		else
		{
			new_type_name = "text";
			type = MakeUnique<InputTypeText>(this);
		}

		if (!type_name.empty())
			SetClass(type_name, false);
		SetClass(new_type_name, true);
		type_name = new_type_name;

		DirtyLayout();
	}

	RMLUI_ASSERT(type);

	if (!type->OnAttributeChange(changed_attributes))
		DirtyLayout();
}

// Called when properties on the element are changed.
void ElementFormControlInput::OnPropertyChange(const PropertyIdSet& changed_properties)
{
	ElementFormControl::OnPropertyChange(changed_properties);

	if (type)
		type->OnPropertyChange(changed_properties);
}

// If we are the added element, this will pass the call onto our type handler.
void ElementFormControlInput::OnChildAdd(Element* child)
{
	if (child == this && type)
		type->OnChildAdd();
}

// If we are the removed element, this will pass the call onto our type handler.
void ElementFormControlInput::OnChildRemove(Element* child)
{
	if (child == this && type)
		type->OnChildRemove();
}

// Handles the "click" event to toggle the control's checked status.
void ElementFormControlInput::ProcessDefaultAction(Event& event)
{
	ElementFormControl::ProcessDefaultAction(event);
	if (type)
		type->ProcessDefaultAction(event);
}

bool ElementFormControlInput::GetIntrinsicDimensions(Vector2f& dimensions, float& ratio)
{
	if (type)
		return type->GetIntrinsicDimensions(dimensions, ratio);
	return false;
}

} // namespace Rml
