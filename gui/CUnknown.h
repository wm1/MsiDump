/////////////////////////////////////////////////////////////////////////
//
// CUnknown.h
//
// this file defines two macro to help implementing IUnknown.
//
// one is:
//     DECLARE_IUNKNOWN
//
// and the other is
//     INIT_IUNKNOWN(iid)
//
/////////////////////////////////////////////////////////////////////////
//
// Usage:
//
// DECLARE_IUNKNOWN should be placed at
//
//     class CXxx : public IXxx
//     {
//         DECLARE_IUNKNOWN;
//         ...
//     };
//
// INIT_IUNKNOWN should be placed at
//
//     CXxx::CXxx(param)
//         : INIT_IUNKNOWN(IID_IXxx)
//     {
//         ...
//     }
//
// Finally, write something like this to instance the object:
//
// IXxx* CreateInstance()
// {
//    IXxx *pIXxx = (IXxx*) new CXxx(param);
//    return pIXxx;
// }
//
// or (the same, but more completely):
//
// IXxx* CreateInstance()
// {
//    IXxx *pIXxx = NULL;
//    CXxx *obj  = new CXxx(param);
//    hr = obj->QueryInterface(IID_IXxx, &pIXxx);
//    obj->Release();
//    return pIXxx;
// }
//
// when finished, call pIXxx->Release() and the object will be deleted.
//
/////////////////////////////////////////////////////////////////////////

#define DECLARE_IUNKNOWN                                                \
                                                                        \
private:                                                                \
        ULONG  refCount;                                                \
        REFIID iid;                                                     \
                                                                        \
public:                                                                 \
        STDMETHOD (QueryInterface)(REFIID _iid, void **p)               \
        {                                                               \
                if (_iid != IID_IUnknown && _iid != iid)                \
                {                                                       \
                        *p = NULL;                                      \
                        return E_NOINTERFACE;                           \
                }                                                       \
                *p = this;                                              \
                AddRef();                                               \
                return S_OK;                                            \
        }                                                               \
                                                                        \
        STDMETHOD_(ULONG, AddRef )()                                    \
        {                                                               \
                return ++refCount;                                      \
        }                                                               \
                                                                        \
        STDMETHOD_(ULONG, Release)()                                    \
        {                                                               \
                ULONG i = --refCount;                                   \
                if (refCount == 0)                                      \
                        delete this;                                    \
                return i;                                               \
        }

/////////////////////////////////////////////////////////////////////////

#define INIT_IUNKNOWN(_iid)                                             \
                                                                        \
        iid(_iid),                                                      \
        refCount(1)

/////////////////////////////////////////////////////////////////////////
