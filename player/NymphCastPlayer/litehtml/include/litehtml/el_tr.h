#ifndef LH_EL_TR_H
#define LH_EL_TR_H

#include "html_tag.h"

namespace litehtml
{
	class el_tr : public html_tag
	{
	public:
		el_tr(const std::shared_ptr<litehtml::document>& doc);
		virtual ~el_tr();

		virtual void	parse_attributes() override;
		virtual void	get_inline_boxes(position::vector& boxes) override;
	};
}

#endif  // LH_EL_TR_H
