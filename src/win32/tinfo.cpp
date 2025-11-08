#include "pch.h"

#ifdef _WIN32
#include "win32/tinfo.h"

namespace typelib {

	std::wstring guid_to_string(const GUID& guid)
	{
		wchar_t buf[256];
		size_t len = ::StringFromGUID2(guid, buf, 256);
		if (len == 0)
		{
			return L"";
		}
		return std::wstring(buf, len - 1);
	}

	GUID string_to_guid(const std::wstring& str)
	{
		GUID guid;
		::CLSIDFromString(str.c_str(), &guid);
		return guid;
	}

	std::wstring new_guid()
	{
		GUID guid;
		::CoCreateGuid(&guid);
		return guid_to_string(guid);
	}

	const wchar_t* varname(int vt)
	{
		static const wchar_t* varnames[] = {
			L"VT_EMPTY",
			L"VT_NULL",
			L"short", //"VT_I2",
			L"long", //"VT_I4",
			L"float", //"VT_R4",
			L"double", //"VT_R8",
			L"CY", //"VT_CY",
			L"DATE", //"VT_DATE",
			L"BSTR", //"VT_BSTR",
			L"IDispatch*", //"VT_DISPATCH",
			L"VT_ERROR",
			L"VARIANT_BOOL", //"VT_BOOL",
			L"VARIANT", //"VT_VARIANT",
			L"IUnknown*", //"VT_UNKNOWN",
			L"VT_DECIMAL",
			L"!!!! ERRR",
			L"char", //"VT_I1",
			L"unsigned char", //"""VT_UI1",
			L"unsigned short",
			L"unsigned long",
			L"longlong",
			L"unsinged longlong",
			L"int",
			L"unsigned int",
			L"void",
			L"HRESULT",
			L"VT_PTR",
			L"SAFEARRAY",
			L"VT_CARRAY",
			L"VT_USERDEFINED",
			L"char",
			L"wchar_t",
			L"VT_RECORD",
			L"int*",
			L"unsigned int *",
			L"FILETIME",
			L"VT_BLOB",
			L"IStream",
			L"IStorage",
			L"VT_STREAMED_OBJECT",
			L"VT_STORED_OBJECT",
			L"VT_BLOB_OBJECT",
			L"VT_CF",
			L"GUID",
			L"VT_VERSIONED_STREAM",
			L"VT_BSTR_BLOB",
			L"VT_VECTOR",
			L"VT_ARRAY",
			L"VT_BYREF",
			L"VT_RESERVED",
			L"VT_ILLEGAL",
			L"VT_ILLEGALMASKED",
			L"VT_TYPEMASK"
		};

		return varnames[vt];
	}

	std::wstring getName(ITypeInfo* typeInfo, int index)
	{
		BSTR name = 0;
		if (S_OK == typeInfo->GetDocumentation(index, &name, NULL, NULL, NULL))
		{
			UINT len = SysStringLen(name);
			std::wstring s((wchar_t*)name, len);
			::SysFreeString(name);
			return s;
		}
		return L"";
	}

	std::wstring getDescription(ITypeInfo* typeInfo, int index)
	{
		BSTR desc = 0;
		if (S_OK == typeInfo->GetDocumentation(index, NULL, &desc, NULL, NULL))
		{
			UINT len = SysStringLen(desc);
			std::wstring s((wchar_t*)desc, len);
			::SysFreeString(desc);
			return s;
		}
		return L"";
	}

	std::wstring reftypeName(ITypeInfo* type_info, HREFTYPE& rt, int index )
	{
		TYPEATTR* attr = nullptr;

		if (S_OK != type_info->GetTypeAttr(&attr))
		{
			return L"";
		}

		ITypeInfo* t = nullptr;
		if (S_OK == type_info->GetRefTypeInfo(rt, &t))
		{
			BSTR fname = 0;
			t->GetDocumentation(index, &fname, NULL, 0, NULL);
			UINT len = SysStringLen(fname);
			std::wstring s((wchar_t*)fname, len);
			::SysFreeString(fname);
			t->Release();
			return s;
		}
		return L"";
	}

	bool isPointer(ITypeInfo* /* type_info */, TYPEDESC* td)
	{
		bool result = false;
		VARTYPE vt = td->vt;
		if (vt == VT_PTR)
		{
			result = true;
		}
		return result;
	}

	bool isPointerPointer(ITypeInfo* /* type_info */, TYPEDESC* td)
	{
		bool result = false;
		VARTYPE vt = td->vt;
		if (vt == VT_PTR)
		{
			vt = td->lptdesc->vt;
			if (vt == VT_PTR)
			{
				result = true;
			}
		}
		return result;
	}

	std::wstring getType(ITypeInfo* type_info, TYPEDESC* td)
	{
		bool isPointer = false;
		bool isPointerPointer = false;

		VARTYPE vt = td->vt;
		if (vt == VT_PTR)
		{
			isPointer = true;
			vt = td->lptdesc->vt;
			if (vt == VT_PTR)
			{
				isPointerPointer = true;
				vt = td->lptdesc->lptdesc->vt;
				if (vt == VT_USERDEFINED)
				{
					return reftypeName(type_info, td->lptdesc->lptdesc->hreftype) + L"**";
				}
			}
			else
			{
				if (vt == VT_USERDEFINED)
				{
					return reftypeName(type_info, td->lptdesc->hreftype) + L"*";
				}
			}
		}
		else
		{
			if (vt == VT_USERDEFINED)
			{
				return reftypeName(type_info, td->hreftype);
			}
		}
		std::wstring ret = varname(vt);
		if (isPointer)
			ret += L"*";
		if (isPointerPointer)
			ret += L"*";
		return ret;
	}


	void parse_funcdesc(ITypeInfo* typeInfo, typelib::MetaInterface& itf, int fi)
	{
		TYPEATTR* attr = nullptr;
		int cImplTypes = 0;
		GUID guid;

		if (S_OK == typeInfo->GetTypeAttr(&attr))
		{
			cImplTypes = attr->cImplTypes;
			guid = attr->guid;
			typeInfo->ReleaseTypeAttr(attr);
		}

		if (::IsEqualGUID(guid, IID_IDispatch))
		{
			return;
		}

		/*
		// impl types
		for (int i = 0; i < cImplTypes; i++)
		{
			HREFTYPE rt;
			typeInfo->GetRefTypeOfImplType(i, &rt);
			if (rt)
			{
				ITypeInfo* t = nullptr;
				if (S_OK == typeInfo->GetRefTypeInfo(rt, &t))
				{
					TYPEATTR* attr;
					int cFuncs = 0;
					if (S_OK == t->GetTypeAttr(&attr))
					{
						cFuncs = attr->cFuncs;
						for (int j = 0; j < cFuncs; j++)
						{
							parse_funcdesc(t, itf, j);
						}
						t->ReleaseTypeAttr(attr);
					}
					t->Release();
				}
			}
		}
		*/

		FUNCDESC* fd = nullptr;
		typeInfo->GetFuncDesc(fi, &fd);

		if (!fd) return;

//      not a good idea when parsing pure IDispatch
//		if (fd->oVft < 7*sizeof(size_t)) return;

		std::wstring name = getName(typeInfo, fd->memid);

		std::wstring retType = getType(typeInfo, &(fd->elemdescFunc.tdesc));

		typelib::MetaFunc metafunc(name, retType);

		std::wstring funKind = L"";
		if (fd->funckind == FUNC_VIRTUAL || fd->funckind == FUNC_PUREVIRTUAL)
		{
			funKind = L"virtual";
		}
		metafunc.funkind = funKind;

		if (fd->callconv == CC_STDCALL)
		{
			metafunc.isStdCall = true;
		}

		metafunc.invokeKind = fd->invkind;
		int k = fd->invkind;
		switch (k)
		{
		case INVOKE_PROPERTYGET:
		{
			metafunc.invokeKind = DISPATCH_PROPERTYGET;
			metafunc.isPropGet = true;
			break;
		}
		case INVOKE_PROPERTYPUT:
		{
			metafunc.invokeKind = DISPATCH_PROPERTYPUT;
			metafunc.isPropPut = true;
			break;
		}
		case INVOKE_PROPERTYPUTREF:
		{
			metafunc.isPropPutRef = true;
			break;
		}
		}

		UINT cnames = fd->cParams + 1;

		BSTR* names = new BSTR[cnames];

		if (S_OK != typeInfo->GetNames(fd->memid, (BSTR*)names, cnames, &cnames))
		{
			typeInfo->ReleaseFuncDesc(fd);
			delete[] names;
			return;
		}

		for (unsigned int j = 1; j < cnames; j++)
		{
			std::wstring pType = getType(typeInfo, &(fd->lprgelemdescParam[j - 1].tdesc));

			UINT len = SysStringLen(names[j]);
			std::wstring s((wchar_t*)names[j], len);

			typelib::MetaParam metaparam(s, pType, fd->lprgelemdescParam[j - 1].tdesc.vt);

			metaparam.isPointer = isPointer(typeInfo, &(fd->lprgelemdescParam[j - 1].tdesc));
			metaparam.isPointerPointer = isPointerPointer(typeInfo, &(fd->lprgelemdescParam[j - 1].tdesc));

			if (fd->lprgelemdescParam[j - 1].paramdesc.wParamFlags & PARAMFLAG_FRETVAL)
			{
				metaparam.isRetVal = true;
			}
			if (fd->lprgelemdescParam[j - 1].paramdesc.wParamFlags & PARAMFLAG_FIN)
			{
				metaparam.isIn = true;
			}
			if (fd->lprgelemdescParam[j - 1].paramdesc.wParamFlags & PARAMFLAG_FOUT)
			{
				metaparam.isOut = true;
			}
			metafunc.params.push_back(metaparam);
		}

		for (unsigned int j = cnames - 1; j < (unsigned int)fd->cParams; j++)
		{
			std::wstring pType = getType(typeInfo, &(fd->lprgelemdescParam[j].tdesc));
			std::wostringstream oss;
			oss << L"param" << j;

			typelib::MetaParam metaparam(oss.str(), pType, fd->lprgelemdescParam[j].tdesc.vt);

			metaparam.isPointer = isPointer(typeInfo, &(fd->lprgelemdescParam[j].tdesc));
			metaparam.isPointerPointer = isPointerPointer(typeInfo, &(fd->lprgelemdescParam[j].tdesc));


			if (fd->lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FRETVAL)
			{
				metaparam.isRetVal = true;
			}
			if (fd->lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FIN)
			{
				metaparam.isIn = true;
			}
			if (fd->lprgelemdescParam[j].paramdesc.wParamFlags & PARAMFLAG_FOUT)
			{
				metaparam.isOut = true;
			}
			metafunc.params.push_back(metaparam);
		}

		if (itf.isDisp)
		{
			metafunc.dispid = fd->memid;
		}
		metafunc.vtindex = fd->oVft;

		metafunc.desc = getDescription(typeInfo, fd->memid);

		typeInfo->ReleaseFuncDesc(fd);

		itf.functions.push_back(metafunc);

		for (size_t i = 0; i < cnames; i++)
		{
			::SysFreeString(names[i]);
		}

		delete[] names;
	}

	void parseRefTypes(ITypeInfo* typeInfo, typelib::MetaInterface& itf)
	{
		TYPEATTR* attr = nullptr;
		int cVars = 0;
		int cImplTypes = 0;
		GUID guid;

		if (S_OK == typeInfo->GetTypeAttr(&attr))
		{
			cImplTypes = attr->cImplTypes;
			guid = attr->guid;
			cVars = attr->cVars;
			typeInfo->ReleaseTypeAttr(attr);
		}

		if (::IsEqualGUID(guid, IID_IDispatch))
		{
			return;
		}

		// impl types
		for (int i = 0; i < cImplTypes; i++)
		{
			HREFTYPE rt;
			typeInfo->GetRefTypeOfImplType(i, &rt);
			if (rt)
			{
				ITypeInfo* t = nullptr;
				if (S_OK == typeInfo->GetRefTypeInfo(rt, &t))
				{
					TYPEATTR* a = nullptr;
					int cFuns = 0;
					if (S_OK == t->GetTypeAttr(&a))
					{
						cFuns = a->cFuncs;
						for (int j = 0; j < cFuns; j++)
						{
							parse_funcdesc(t, itf, j);
						}
						t->ReleaseTypeAttr(a);
					}
					parseRefTypes(t, itf);
					t->Release();
				}
			}
		}
	}



	bool parse_typeinfo(ITypeInfo* typeInfo,std::wstring /*name*/, typelib::MetaInterface& result)
	{
		TYPEATTR* attr;
		int cFuncs = 0;
		int cVars = 0;
		int cImplTypes = 0;
		int wTypeFlags = 0;
		GUID guid;

		if (S_OK != typeInfo->GetTypeAttr(&attr))
		{
			return false;
		}

		cVars = attr->cVars;
		cFuncs = attr->cFuncs;
		guid = attr->guid;
		cImplTypes = attr->cImplTypes;
		wTypeFlags = attr->wTypeFlags;

		typeInfo->ReleaseTypeAttr(attr);

//		std::wstring base;

		// impl types
		for (int i = 0; i < cImplTypes; i++)
		{
			HREFTYPE rt;
			typeInfo->GetRefTypeOfImplType(i, &rt);
			if (rt)
			{
				result.base = reftypeName(typeInfo, rt);
			}
		}

		HREFTYPE rt;
		if (S_OK == typeInfo->GetRefTypeOfImplType( (UINT) - 1, &rt))
		{
			if (rt)
			{
				result.isDual = true;
				result.isDisp = true;

				ITypeInfo* ti = nullptr;
				if (S_OK == typeInfo->GetRefTypeInfo(rt, &ti))
				{
					TYPEATTR* a = nullptr;
					int cFuns = 0;

					if (S_OK == ti->GetTypeAttr(&a))
					{
						cFuns = a->cFuncs;
						for (int i = 0; i < cFuns; i++)
						{
							parse_funcdesc(ti, result, i);
						}
						ti->ReleaseTypeAttr(a);
						parseRefTypes(ti, result);
					}

				}
			}
		}

		// Dispatcheria	
		if (!result.isDual)
		{
			if (wTypeFlags & TYPEFLAG_FDUAL)
			{
				result.isDual = true;
				for (int i = 7; i < cFuncs; i++)
				{
					parse_funcdesc(typeInfo, result, i);
				}

			}
			else
			{
				for (int i = 0; i < cFuncs; i++)
				{
					parse_funcdesc(typeInfo, result, i);
				}
				for (int i = 0; i < cVars; i++)
				{
					VARDESC* vd;
					typeInfo->GetVarDesc(i, &vd);

					typelib::MetaParam metaparam(
						getName(typeInfo, vd->memid), 
						getType(typeInfo, &vd->elemdescVar.tdesc), 
						vd->elemdescVar.tdesc.vt
					);
					metaparam.dispid = vd->memid;

					typeInfo->ReleaseVarDesc(vd);

					result.properties.push_back(metaparam);
				}
			}
		}

		return true;
	}

}

#endif

