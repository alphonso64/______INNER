#ifndef M_AUTOBUFFER
#define M_AUTOBUFFER
template<typename _Tp, size_t fixed_size = 1024/sizeof(_Tp)+8> class mAutoBuffer
{
public:
    typedef _Tp value_type;

    //! the default constructor
    mAutoBuffer();
    //! constructor taking the real buffer size
    mAutoBuffer(size_t _size);

    //! the copy constructor
    mAutoBuffer(const mAutoBuffer<_Tp, fixed_size>& buf);
    //! the assignment operator
    mAutoBuffer<_Tp, fixed_size>& operator = (const mAutoBuffer<_Tp, fixed_size>& buf);

    //! destructor. calls deallocate()
    ~mAutoBuffer();

    //! allocates the new buffer of size _size. if the _size is small enough, stack-allocated buffer is used
    void allocate(size_t _size);
    //! deallocates the buffer if it was dynamically allocated
    void deallocate();
    //! resizes the buffer and preserves the content
    void resize(size_t _size);
    //! returns the current buffer size
    size_t size() const;
    //! returns pointer to the real buffer, stack-allocated or head-allocated
    operator _Tp* ();
    //! returns read-only pointer to the real buffer, stack-allocated or head-allocated
    operator const _Tp* () const;

protected:
    //! pointer to the real buffer, can point to buf if the buffer is small enough
    _Tp* ptr;
    //! size of the real buffer
    size_t sz;
    //! pre-allocated buffer. At least 1 element to confirm C++ standard reqirements
    _Tp buf[(fixed_size > 0) ? fixed_size : 1];
};




template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::mAutoBuffer()
{
    ptr = buf;
    sz = fixed_size;
}

template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::mAutoBuffer(size_t _size)
{
    ptr = buf;
    sz = fixed_size;
    allocate(_size);
}

template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::mAutoBuffer(const mAutoBuffer<_Tp, fixed_size>& abuf )
{
    ptr = buf;
    sz = fixed_size;
    allocate(abuf.size());
    for( size_t i = 0; i < sz; i++ )
        ptr[i] = abuf.ptr[i];
}

template<typename _Tp, size_t fixed_size> inline mAutoBuffer<_Tp, fixed_size>&
mAutoBuffer<_Tp, fixed_size>::operator = (const mAutoBuffer<_Tp, fixed_size>& abuf)
{
    if( this != &abuf )
    {
        deallocate();
        allocate(abuf.size());
        for( size_t i = 0; i < sz; i++ )
            ptr[i] = abuf.ptr[i];
    }
    return *this;
}

template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::~mAutoBuffer()
{ deallocate(); }

template<typename _Tp, size_t fixed_size> inline void
mAutoBuffer<_Tp, fixed_size>::allocate(size_t _size)
{
    if(_size <= sz)
    {
        sz = _size;
        return;
    }
    deallocate();
    if(_size > fixed_size)
    {
        ptr = new _Tp[_size];
        sz = _size;
    }
}

template<typename _Tp, size_t fixed_size> inline void
mAutoBuffer<_Tp, fixed_size>::deallocate()
{
    if( ptr != buf )
    {
        delete[] ptr;
        ptr = buf;
        sz = fixed_size;
    }
}

template<typename _Tp, size_t fixed_size> inline void
mAutoBuffer<_Tp, fixed_size>::resize(size_t _size)
{
    if(_size <= sz)
    {
        sz = _size;
        return;
    }
    size_t i, prevsize = sz, minsize = MIN(prevsize, _size);
    _Tp* prevptr = ptr;

    ptr = _size > fixed_size ? new _Tp[_size] : buf;
    sz = _size;

    if( ptr != prevptr )
        for( i = 0; i < minsize; i++ )
            ptr[i] = prevptr[i];
    for( i = prevsize; i < _size; i++ )
        ptr[i] = _Tp();

    if( prevptr != buf )
        delete[] prevptr;
}

template<typename _Tp, size_t fixed_size> inline size_t
mAutoBuffer<_Tp, fixed_size>::size() const
{ return sz; }

template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::operator _Tp* ()
{ return ptr; }

template<typename _Tp, size_t fixed_size> inline
mAutoBuffer<_Tp, fixed_size>::operator const _Tp* () const
{ return ptr; }
#endif