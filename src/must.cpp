#include "pch.h"
#include "must.h"
#include "object.h"

mustache::mustache( const std::string& tpl)
	: template_(tpl)
{}

std::string mustache::render(Value data)
{
	return render(template_,data);
}


std::string mustache::render(Data& data)
{
	return render(template_,data);
}

std::string mustache::render(const std::string& tpl, Value data)
{
	Data d(fromValue(data));
	return render(tpl,d);
}


std::string mustache::render(const std::string& tpl, Data& data)
{
	std::ostringstream oss;

	Mustache tmpl{tpl};

	tmpl.render(data, oss);

	return oss.str();
}

mustache::Data mustache::fromValue(Value& data)
{
    if(IS_NIL(data))
    {
        return Data(Data::type::bool_false);
    }
    if(IS_BOOL(data))
    {
        return Data(data.as.boolean);
    }
    if(IS_INT(data))
    {
        return Data(data.to_string());
    }
    if(IS_NUMBER(data))
    {
        return Data(data.to_string());
    }
    
    if(IS_OBJ(data))
    {
        auto str = as<ObjString>(data.as.obj);
        if(str)
        {
            return Data(str->toString());
        }
        auto array = as<Indexable>(data.as.obj);
        if(array)
        {
            unsigned int len = (unsigned int)array->size().to_int();
            Data d(Data::type::list);
            for( unsigned int i = 0; i < len; i++)
            {
                auto value = array->index(i);
                d.push_back( fromValue( value ));
            }
            return d;
        }
        auto map = as<Propertyable>(data.as.obj);
        if(map)
        {
            Data d(Data::type::object);
            auto keys = map->keys();
            for(auto& key : keys)
            {
                Value value = map->getProperty(key);
                d.set( key, fromValue(value) );
            }
            return d;
        }

        return Data(data.as.obj->toString());
    }
    return Data(Data::type::bool_false);
}
