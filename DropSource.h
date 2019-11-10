#ifndef _DROPSOURCE_H
#define _DROPSOURCE_H

#include <ole2.h>

class CDropSource : public IDropSource
{
public:
  // *** IUnknown ***
  STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
  STDMETHODIMP_(ULONG) AddRef();
  STDMETHODIMP_(ULONG) Release();

  // *** IDropSource ***
  STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState);
  STDMETHODIMP GiveFeedback(DWORD dwEffect);

  CDropSource() : m_cRef(1) { }
private:
  ULONG m_cRef;
};

#endif
