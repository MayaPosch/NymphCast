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

#include "StyleSheetNodeSelectorLastChild.h"
#include "../../Include/RmlUi/Core/ElementText.h"

namespace Rml {

StyleSheetNodeSelectorLastChild::StyleSheetNodeSelectorLastChild()
{
}

StyleSheetNodeSelectorLastChild::~StyleSheetNodeSelectorLastChild()
{
}

// Returns true if the element is the last DOM child in its parent.
bool StyleSheetNodeSelectorLastChild::IsApplicable(const Element* element, int RMLUI_UNUSED_PARAMETER(a), int RMLUI_UNUSED_PARAMETER(b))
{
	RMLUI_UNUSED(a);
	RMLUI_UNUSED(b);

	Element* parent = element->GetParentNode();
	if (parent == nullptr)
		return false;

	int child_index = parent->GetNumChildren() - 1;
	while (child_index >= 0)
	{
		// If this child (the last non-text child) is our element, then the selector succeeds.
		Element* child = parent->GetChild(child_index);
		if (child == element)
			return true;

		// If this child is not a text element, then the selector fails; this element is non-trivial.
		if (rmlui_dynamic_cast< ElementText* >(child) == nullptr &&
			child->GetDisplay() != Style::Display::None)
			return false;

		// Otherwise, skip over the text element to find the last non-trivial element.
		child_index--;
	}

	return false;
}

} // namespace Rml
