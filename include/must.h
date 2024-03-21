#ifndef _MOC_MUST_DEFINE_GUARD_DEFINE_
#define _MOC_MUST_DEFINE_GUARD_DEFINE_

#include "value.h"
#include "kainjow/mustache.hpp"

/*
	SImple Mustache wrapper
*/

class mustache
{
public:

	using Mustache = kainjow::mustache::mustache;
	using Data = kainjow::mustache::data;

	// constrcut mustache template from string
	mustache( const std::string& tpl);

	std::string render(Data& data);
	// render template with data from JSON
	std::string render(Value data);

	static std::string render(const std::string& tpl,Data& data);
	
	// render template with data from JSON
	static std::string render(const std::string& tpl,Value data);

	static Data fromValue(Value& data);

private:
	std::string template_;
};

#endif
