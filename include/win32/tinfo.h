#ifndef moc_tinfo_h
#define moc_tinfo_h

#ifdef _WIN32

#include <windows.h>
#include <OAIdl.h>
#include <string>
#include <vector>
#include <sstream>

/*
	MOSER support for IDispatch type libraries
*/

namespace typelib {

	std::wstring guid_to_string(const GUID& guid);
	GUID string_to_guid(const std::wstring& str);
	std::wstring new_guid();

	class MetaParam
	{
	public:

		MetaParam() {};
		MetaParam(const std::wstring& n, const std::wstring& t, VARTYPE v)
			: name(n), type(t), vt(v)
		{};

		std::wstring name;
		std::wstring type;
		VARTYPE vt;

		int dispid;

		bool isPointer = false;
		bool isPointerPointer = false;
		bool isRetVal = false;
		bool isIn = false;
		bool isOut = false;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class MetaFunc
	{
	public:

		MetaFunc() {};
		MetaFunc(const std::wstring& n, const std::wstring& t)
			: name(n), type(t)
		{};

		VARTYPE vt_result;

		std::wstring name;
		std::wstring type;
		std::wstring funkind;
		std::wstring desc;

		bool isDisp = false;
		bool isStdCall = false;
		bool isPropPut = false;
		bool isPropPutRef = false;
		bool isPropGet = false;

		long dispid = 0;
		long vtindex = 0;
		long invokeKind = 0;
		std::vector<MetaParam> params;
	};

	//////////////////////////////////////////////////////////////////////////////////////////

	class MetaInterface
	{
	public:

		MetaInterface() {};
		MetaInterface(const std::wstring& t, const std::wstring& b, GUID& g)
			: type(t), base(b), guid(guid_to_string(g))
		{}

		std::wstring type;
		std::wstring base;
		std::wstring desc;
		std::wstring guid;

		bool isDisp = false;
		bool isDual = false;

		std::vector<MetaFunc> functions;
		std::vector<MetaParam> properties;
	};

	class MetaInterfaceRef
	{
	public:

		MetaInterfaceRef() {};
		MetaInterfaceRef(const std::wstring& t)
			: type(t)
		{}

		std::wstring type;

		bool isSource = false;
		bool isDefault = false;
	};

	const wchar_t* varname(int vt);

	std::wstring getName(ITypeInfo* typeInfo, int index);
	std::wstring getDescription(ITypeInfo* typeInfo, int index);
	std::wstring reftypeName(ITypeInfo* type_info, HREFTYPE& rt, int index = MEMBERID_NIL);

	bool isPointer(ITypeInfo* type_info, TYPEDESC* td);
	bool isPointerPointer(ITypeInfo* type_info, TYPEDESC* td);

	std::wstring getType(ITypeInfo* type_info, TYPEDESC* td);

	void parse_funcdesc(ITypeInfo* typeInfo, typelib::MetaInterface& itf, int fi);
	void parseRefTypes(ITypeInfo* typeInfo, typelib::MetaInterface& itf);

	bool parse_typeinfo(ITypeInfo* typeInfo,std::wstring name, typelib::MetaInterface& result );
 
}

#endif
#endif
