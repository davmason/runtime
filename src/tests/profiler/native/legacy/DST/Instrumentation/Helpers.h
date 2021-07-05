class IUnknownHolder
{
public:
    IUnknownHolder (IUnknown *pUnknown)
    {
        m_pUnknown = pUnknown;
    }

    ~IUnknownHolder ()
    {
        m_pUnknown->Release();
    }
private:
    IUnknown *m_pUnknown;
};
