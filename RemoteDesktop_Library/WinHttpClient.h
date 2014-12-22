/**
 *  Copyright 2008-2010 Cheng Shi.  All rights reserved.
 *  Email: shicheng107@hotmail.com
 */

#ifndef WINHTTPCLIENT_H
#define WINHTTPCLIENT_H
#include <string>
#include <Winhttp.h>

class WinHttpClient
{
public:
	typedef bool (*PROGRESSPROC)(double);
     WinHttpClient(const std::wstring &url, PROGRESSPROC progressProc = NULL);
     ~WinHttpClient();

    // It is a synchronized method and may take a long time to finish.
     bool SendHttpRequest(const std::wstring &httpVerb = L"GET", bool disableAutoRedirect = false);
     std::wstring GetResponseHeader();
     std::wstring GetResponseContent();
     std::wstring GetResponseCharset();
     std::wstring GetResponseStatusCode();
     std::wstring GetResponseLocation();
     std::wstring GetRequestHost();
     const BYTE *GetRawResponseContent();
     unsigned int GetRawResponseContentLength();
     unsigned int GetRawResponseReceivedContentLength();
     bool SaveResponseToFile(const std::wstring &filePath);
     std::wstring GetResponseCookies();
     bool SetAdditionalRequestCookies(const std::wstring &cookies);
     bool SetAdditionalDataToSend(BYTE *data, unsigned int dataSize);
     bool UpdateUrl(const std::wstring &url);
     bool ResetAdditionalDataToSend();
     bool SetAdditionalRequestHeaders(const std::wstring &additionalRequestHeaders);
     bool SetRequireValidSslCertificates(bool require);
     bool SetProxy(const std::wstring &proxy);
     DWORD GetLastError();
     bool SetUserAgent(const std::wstring &userAgent);
     bool SetForceCharset(const std::wstring &charset);
     bool SetProxyUsername(const std::wstring &username);
     bool SetProxyPassword(const std::wstring &password);
     bool SetTimeouts(unsigned int resolveTimeout = 0,
                            unsigned int connectTimeout = 60000,
                            unsigned int sendTimeout = 30000,
                            unsigned int receiveTimeout = 30000);

private:
     WinHttpClient(const WinHttpClient &other);
     WinHttpClient &operator =(const WinHttpClient &other);
     bool SetProgress(unsigned int byteCountReceived);

    HINTERNET m_sessionHandle;
    bool m_requireValidSsl;
    std::wstring m_requestURL;
    std::wstring m_requestHost;
    std::wstring m_responseHeader;
    std::wstring m_responseContent;
    std::wstring m_responseCharset;
    BYTE *m_pResponse;
    unsigned int m_responseByteCountReceived;   // Up to 4GB.
    PROGRESSPROC m_pfProcessProc;
    unsigned int m_responseByteCount;
    std::wstring m_responseCookies;
    std::wstring m_additionalRequestCookies;
    BYTE *m_pDataToSend;
    unsigned int m_dataToSendSize;
    std::wstring m_additionalRequestHeaders;
    std::wstring m_proxy;
    DWORD m_dwLastError;
    std::wstring m_statusCode;
    std::wstring m_userAgent;
    bool m_bForceCharset;
    std::wstring m_proxyUsername;
    std::wstring m_proxyPassword;
    std::wstring m_location;
    unsigned int m_resolveTimeout;
    unsigned int m_connectTimeout;
    unsigned int m_sendTimeout;
    unsigned int m_receiveTimeout;
};

#endif // WINHTTPCLIENT_H
