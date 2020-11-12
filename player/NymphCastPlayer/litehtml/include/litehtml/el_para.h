#ifndef LH_EL_PARA_H
#define LH_EL_PARA_H

#include "html_tag.h"

namespace litehtml
{
	class el_para : public html_tag
	{
	public:
		el_para(const std::shared_ptr<litehtml::document>& doc);
		virtual ~el_para();

		virtual void	parse_attributes() override;

	};
}

#endif  // LH_EL_PARA_H
