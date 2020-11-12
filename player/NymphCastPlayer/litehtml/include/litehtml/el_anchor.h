#ifndef LH_EL_ANCHOR_H
#define LH_EL_ANCHOR_H

#include "html_tag.h"

namespace litehtml
{
	class el_anchor : public html_tag
	{
	public:
		el_anchor(const std::shared_ptr<litehtml::document>& doc);
		virtual ~el_anchor();

		virtual void	on_click() override;
		virtual void	apply_stylesheet(const litehtml::css& stylesheet) override;
	};
}

#endif  // LH_EL_ANCHOR_H
