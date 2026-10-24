#include "pch.h"

#if defined(_DEBUG) && !defined(NDEBUG)
	#define new DEBUG_NEW
#endif

namespace CGDK
{

CGMFCCONTROL_EXT_API CStatic_graph* WINAPI CGCreateControlStaticGraph(int _data_length, int _GridUnit)
{
	// Return) 생성된 윈도우 리턴~
	return new CStatic_graph(_data_length, _GridUnit);
}

CGMFCCONTROL_EXT_API INT_PTR WINAPI DoModalObjectFactoryStatus(factory::manager* _pmanager)
{
	// 1) Dialog를 생성한다.
	CDialog_object_factory_status dlg(_pmanager);

	// 2) DoModal해서 결과를 Return한다.
	return dlg.DoModal();
}

CGMFCCONTROL_EXT_API INT_PTR WINAPI	DoModalThreadStatus(api::manager_thread* _pmanager)
{
	// 1) Dialog를 생성한다.
	CDialog_thread_status dlg(_pmanager);

	// 2) DoModal해서 결과를 Return한다.
	return dlg.DoModal();
}

CGMFCCONTROL_EXT_API INT_PTR WINAPI	DoModalReferenceStatus(Ireferenceable* _object)
{
	// 1) Dialog를 생성한다.
	CDialog_reference_status dlg(_object);

	// 2) DoModal해서 결과를 Return한다.
	return dlg.DoModal();
}

CGMFCCONTROL_EXT_API INT_PTR WINAPI	DoModalCGDKComponentInfo(const std::vector<CGDK::sCOMPONENT_INFO>& _vector_cgdk_components)
{
	// 1) Dialog를 생성한다.
	CDialog_CGDK_component_info	dlg(_vector_cgdk_components);

	// 2) DoModal해서 결과를 Return한다.
	return dlg.DoModal();
}


}