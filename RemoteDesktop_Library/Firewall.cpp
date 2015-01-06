#include "stdafx.h"
#include "Firewall.h"

#include <crtdbg.h>
#include <netfw.h>
#include <objbase.h>
#include <oleauto.h>
#include <stdio.h>

HRESULT WindowsFirewallInitialize(OUT INetFwProfile** fwProfile)
{
	HRESULT hr = S_OK;
	INetFwMgr* fwMgr = NULL;
	INetFwPolicy* fwPolicy = NULL;

	assert(fwProfile != nullptr);

	*fwProfile = NULL;

	// Create an instance of the firewall settings manager.
	hr = CoCreateInstance(
		__uuidof(NetFwMgr),
		NULL,
		CLSCTX_INPROC_SERVER,
		__uuidof(INetFwMgr),
		(void**)&fwMgr
		);
	if (FAILED(hr))
	{
		DEBUG_MSG("CoCreateInstance failed: %", hr);
		goto error;
	}

	// Retrieve the local firewall policy.
	hr = fwMgr->get_LocalPolicy(&fwPolicy);
	if (FAILED(hr))
	{
		DEBUG_MSG("get_LocalPolicy failed: %", hr);
		goto error;
	}

	// Retrieve the firewall profile currently in effect.
	hr = fwPolicy->get_CurrentProfile(fwProfile);
	if (FAILED(hr))
	{
		DEBUG_MSG("get_CurrentProfile failed: %", hr);
		goto error;
	}

error:

	// Release the local firewall policy.
	if (fwPolicy != NULL)
	{
		fwPolicy->Release();
	}

	// Release the firewall settings manager.
	if (fwMgr != NULL)
	{
		fwMgr->Release();
	}

	return hr;
}
HRESULT WindowsFirewallAppIsEnabled(
	IN INetFwProfile* fwProfile,
	IN const wchar_t* fwProcessImageFileName,
	OUT BOOL* fwAppEnabled
	)
{
	HRESULT hr = S_OK;
	BSTR fwBstrProcessImageFileName = NULL;
	VARIANT_BOOL fwEnabled;
	INetFwAuthorizedApplication* fwApp = NULL;
	INetFwAuthorizedApplications* fwApps = NULL;

	_ASSERT(fwProfile != NULL);
	_ASSERT(fwProcessImageFileName != NULL);
	_ASSERT(fwAppEnabled != NULL);

	*fwAppEnabled = FALSE;

	// Retrieve the authorized application collection.
	hr = fwProfile->get_AuthorizedApplications(&fwApps);
	if (FAILED(hr))
	{
		DEBUG_MSG("get_AuthorizedApplications failed: %", hr);
		goto error;
	}

	// Allocate a BSTR for the process image file name.
	fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
	if (fwBstrProcessImageFileName == NULL)
	{
		hr = E_OUTOFMEMORY;
		DEBUG_MSG("SysAllocString failed: %", hr);
		goto error;
	}

	// Attempt to retrieve the authorized application.
	hr = fwApps->Item(fwBstrProcessImageFileName, &fwApp);
	if (SUCCEEDED(hr))
	{
		// Find out if the authorized application is enabled.
		hr = fwApp->get_Enabled(&fwEnabled);
		if (FAILED(hr))
		{
			DEBUG_MSG("get_Enabled failed: %", hr);
			goto error;
		}

		if (fwEnabled != VARIANT_FALSE)
		{
			// The authorized application is enabled.
			*fwAppEnabled = TRUE;

			DEBUG_MSG(
				"Authorized application % is enabled in the firewall.\n",
				fwProcessImageFileName
				);
		}
		else
		{
			DEBUG_MSG(
				"Authorized application % is disabled in the firewall.\n",
				fwProcessImageFileName
				);
		}
	}
	else
	{
		// The authorized application was not in the collection.
		hr = S_OK;

		DEBUG_MSG(
			"Authorized application % is disabled in the firewall.\n",
			fwProcessImageFileName
			);
	}

error:

	// Free the BSTR.
	SysFreeString(fwBstrProcessImageFileName);

	// Release the authorized application instance.
	if (fwApp != NULL)
	{
		fwApp->Release();
	}

	// Release the authorized application collection.
	if (fwApps != NULL)
	{
		fwApps->Release();
	}

	return hr;
}


HRESULT WindowsFirewallAddApp(
	IN INetFwProfile* fwProfile,
	IN const wchar_t* fwProcessImageFileName,
	IN const wchar_t* fwName
	)
{
	HRESULT hr = S_OK;
	BOOL fwAppEnabled;
	BSTR fwBstrName = NULL;
	BSTR fwBstrProcessImageFileName = NULL;
	INetFwAuthorizedApplication* fwApp = NULL;
	INetFwAuthorizedApplications* fwApps = NULL;

	_ASSERT(fwProfile != NULL);
	_ASSERT(fwProcessImageFileName != NULL);
	_ASSERT(fwName != NULL);

	// First check to see if the application is already authorized.
	hr = WindowsFirewallAppIsEnabled(
		fwProfile,
		fwProcessImageFileName,
		&fwAppEnabled
		);
	if (FAILED(hr))
	{
		DEBUG_MSG("WindowsFirewallAppIsEnabled failed: %", hr);
		goto error;
	}

	// Only add the application if it isn't already authorized.
	if (!fwAppEnabled)
	{
		// Retrieve the authorized application collection.
		hr = fwProfile->get_AuthorizedApplications(&fwApps);
		if (FAILED(hr))
		{
			DEBUG_MSG("get_AuthorizedApplications failed: %", hr);
			goto error;
		}

		// Create an instance of an authorized application.
		hr = CoCreateInstance(
			__uuidof(NetFwAuthorizedApplication),
			NULL,
			CLSCTX_INPROC_SERVER,
			__uuidof(INetFwAuthorizedApplication),
			(void**)&fwApp
			);
		if (FAILED(hr))
		{
			DEBUG_MSG("CoCreateInstance failed: %", hr);
			goto error;
		}

		// Allocate a BSTR for the process image file name.
		fwBstrProcessImageFileName = SysAllocString(fwProcessImageFileName);
		if (fwBstrProcessImageFileName == NULL)
		{
			hr = E_OUTOFMEMORY;
			DEBUG_MSG("SysAllocString failed: %", hr);
			goto error;
		}

		// Set the process image file name.
		hr = fwApp->put_ProcessImageFileName(fwBstrProcessImageFileName);
		if (FAILED(hr))
		{
			DEBUG_MSG("put_ProcessImageFileName failed: %", hr);
			goto error;
		}

		// Allocate a BSTR for the application friendly name.
		fwBstrName = SysAllocString(fwName);
		if (SysStringLen(fwBstrName) == 0)
		{
			hr = E_OUTOFMEMORY;
			DEBUG_MSG("SysAllocString failed: %", hr);
			goto error;
		}

		// Set the application friendly name.
		hr = fwApp->put_Name(fwBstrName);
		if (FAILED(hr))
		{
			DEBUG_MSG("put_Name failed: %", hr);
			goto error;
		}

		// Add the application to the collection.
		hr = fwApps->Add(fwApp);
		if (FAILED(hr))
		{
			DEBUG_MSG("Add failed: %", hr);
			goto error;
		}

		DEBUG_MSG(
			"Authorized application %lS is now enabled in the firewall.\n",
			fwProcessImageFileName
			);
	}

error:

	// Free the BSTRs.
	SysFreeString(fwBstrName);
	SysFreeString(fwBstrProcessImageFileName);

	// Release the authorized application instance.
	if (fwApp != NULL)
	{
		fwApp->Release();
	}

	// Release the authorized application collection.
	if (fwApps != NULL)
	{
		fwApps->Release();
	}

	return hr;
}



RemoteDesktop::WindowsFirewall::WindowsFirewall(){

	HRESULT hr = S_OK;
	// Initialize COM.
	comInit = CoInitializeEx( 0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	// Ignore RPC_E_CHANGED_MODE; this just means that COM has already been
	// initialized with a different mode. Since we don't care what the mode is,
	// we'll just use the existing mode.
	if (comInit != RPC_E_CHANGED_MODE)
	{
		hr = comInit;
		if (FAILED(hr))
		{
			DEBUG_MSG("CoInitializeEx failed: %", hr);
			return;
		}
	}

	// Retrieve the firewall profile currently in effect.
	hr = WindowsFirewallInitialize(&FirewallProfile);
	if (FAILED(hr))
	{
		DEBUG_MSG("WindowsFirewallInitialize failed: %", hr);
	}

}
RemoteDesktop::WindowsFirewall::~WindowsFirewall(){
	if (FirewallProfile != nullptr) FirewallProfile->Release();
	// Uninitialize COM.
	if (SUCCEEDED(comInit))
	{
		CoUninitialize();
	}

}
bool RemoteDesktop::WindowsFirewall::AddProgramException(std::wstring exefullpath, std::wstring name){
	if (FirewallProfile == nullptr) return false;
	return SUCCEEDED(WindowsFirewallAddApp(FirewallProfile, exefullpath.c_str(), name.c_str()));
}




