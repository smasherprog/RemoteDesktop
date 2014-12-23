#include "stdafx.h"
#include "AdminStatus.h"
//Code taken from http://stackoverflow.com/questions/1453497/discover-if-user-has-admin-rights   Have not checked if it is valid yet.... TODO!!!
bool RemoteDesktop::IsUserAdmin(){

	struct Data
	{
		PACL   pACL;
		PSID   psidAdmin;
		HANDLE hToken;
		HANDLE hImpersonationToken;
		PSECURITY_DESCRIPTOR     psdAdmin;
		Data() : pACL(NULL), psidAdmin(NULL), hToken(NULL),
			hImpersonationToken(NULL), psdAdmin(NULL)
		{}
		~Data()
		{
			if (pACL)
				LocalFree(pACL);
			if (psdAdmin)
				LocalFree(psdAdmin);
			if (psidAdmin)
				FreeSid(psidAdmin);
			if (hImpersonationToken)
				CloseHandle(hImpersonationToken);
			if (hToken)
				CloseHandle(hToken);
		}
	} data;

	BOOL   fReturn = FALSE;


	DWORD  dwStructureSize = sizeof(PRIVILEGE_SET);

	PRIVILEGE_SET   ps;
	GENERIC_MAPPING GenericMapping;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	const DWORD ACCESS_READ = 1;
	const DWORD ACCESS_WRITE = 2;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &data.hToken))
	{
		if (GetLastError() != ERROR_NO_TOKEN)
			return false;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE | TOKEN_QUERY, &data.hToken))
			return false;
	}

	if (!DuplicateToken(data.hToken, SecurityImpersonation, &data.hImpersonationToken))
		return false;

	if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &data.psidAdmin))
		return false;

	data.psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (data.psdAdmin == NULL)
		return false;

	if (!InitializeSecurityDescriptor(data.psdAdmin, SECURITY_DESCRIPTOR_REVISION))
		return false;

	// Compute size needed for the ACL.
	auto dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(data.psidAdmin) - sizeof(DWORD);

	data.pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
	if (data.pACL == NULL)
		return false;

	if (!InitializeAcl(data.pACL, dwACLSize, ACL_REVISION2))
		return false;

	DWORD dwAccessMask = ACCESS_READ | ACCESS_WRITE;

	if (!AddAccessAllowedAce(data.pACL, ACL_REVISION2, dwAccessMask, data.psidAdmin))
		return false;

	if (!SetSecurityDescriptorDacl(data.psdAdmin, TRUE, data.pACL, FALSE))
		return false;

	// AccessCheck validates a security descriptor somewhat; set the group
	// and owner so that enough of the security descriptor is filled out 
	// to make AccessCheck happy.

	SetSecurityDescriptorGroup(data.psdAdmin, data.psidAdmin, FALSE);
	SetSecurityDescriptorOwner(data.psdAdmin, data.psidAdmin, FALSE);

	if (!IsValidSecurityDescriptor(data.psdAdmin))
		return false;

	DWORD dwAccessDesired = ACCESS_READ;

	GenericMapping.GenericRead = ACCESS_READ;
	GenericMapping.GenericWrite = ACCESS_WRITE;
	GenericMapping.GenericExecute = 0;
	GenericMapping.GenericAll = ACCESS_READ | ACCESS_WRITE;

	DWORD  dwStatus = 0;
	if (!AccessCheck(data.psdAdmin, data.hImpersonationToken, dwAccessDesired,
		&GenericMapping, &ps, &dwStructureSize, &dwStatus,
		&fReturn))
	{
		return false;
	}

	return fReturn == TRUE;
}