#ifndef __COMXX_H_INCLUDED
#define __COMXX_H_INCLUDED

// github.com/wybosys/nnt.comxx

#define COMXX_NS com
#define COMXX_BEGIN    \
    namespace COMXX_NS \
    {
#define COMXX_END }

#include <atomic>
#include <string>
#include <memory>
#include <functional>
#include <initializer_list>
#include <vector>
#include <map>
#include <cassert>

COMXX_BEGIN

typedef struct {
    unsigned long d1;
    unsigned short d2;
    unsigned short d3;
    union {
        unsigned char d1[8];
        struct {
            unsigned int d1;
            unsigned int d2;
        } d2;
    } d4;
} IID;

static bool operator<(IID const &l, IID const &r) {
    return l.d1 < r.d1 || l.d2 < r.d2 || l.d3 < r.d3 || l.d4.d2.d1 < r.d4.d2.d1 || l.d4.d2.d2 < r.d4.d2.d2;
}

template<typename TObject = class IObject,
        typename TString = ::std::string,
        typename TBytes = ::std::vector<unsigned char>
>
class VariantTypes {
public:
    typedef TObject object_type;
    typedef TString string_type;
    typedef TBytes bytes_type;
};

template<typename Types = VariantTypes<> >
class Variant {
public:

    typedef void (*func_type)();
    typedef typename Types::object_type object_type;
    typedef typename Types::bytes_type bytes_type;
    typedef typename Types::string_type string_type;

    enum struct VT {
        NIL = 0,
        INT = 1,
        UINT = 2,
        LONG = 3,
        ULONG = 4,
        SHORT = 5,
        USHORT = 6,
        LONGLONG = 7,
        ULONGLONG = 8,
        FLOAT = 9,
        DOUBLE = 10,
        CHAR = 11,
        UCHAR = 12,
        BYTES = 13,
        STRING = 14,
        FUNCTION = 15,
        OBJECT = 16,
        BOOLEAN = 17,
        POINTER = 18,
    };

    Variant();

    Variant(object_type *);

    Variant(void *);

    Variant(nullptr_t);

    Variant(int);

    Variant(unsigned int);

    Variant(long);

    Variant(unsigned long);

    Variant(short);

    Variant(unsigned short);

    Variant(long long);

    Variant(unsigned long long);

    Variant(float);

    Variant(double);

    Variant(char);

    Variant(unsigned char);

    Variant(bool);

    Variant(bytes_type const &);

    Variant(::std::string const &);

    Variant(char const *);

    Variant(func_type);

    Variant(Variant const &);

    Variant(::std::shared_ptr<Variant> const &);

    ~Variant();

    const VT vt;

    object_type *toObject() const;

    void *toPointer() const;

    int toInt() const;

    unsigned int toUInt() const;

    long toLong() const;

    unsigned long toULong() const;

    short toShort() const;

    unsigned short toUShort() const;

    long long toLonglong() const;

    unsigned long long toULonglong() const;

    float toFloat() const;

    double toDouble() const;

    char toChar() const;

    unsigned char toUChar() const;

    bool toBool() const;

    bytes_type const &toBytes() const;

    ::std::string const &toString() const;

    func_type toFunction() const;

    Variant &operator=(Variant const &);

private:
    union {
        object_type *o;
        void *p;
        int i;
        unsigned int ui;
        long l;
        unsigned long ul;
        short s;
        unsigned short us;
        long long ll;
        unsigned long long ull;
        float f;
        double d;
        char c;
        unsigned char uc;
        bool b;
        func_type fn;
    } _pod;

    ::std::shared_ptr<bytes_type> _bytes;
    ::std::shared_ptr<string_type> _str;
};

template <typename V>
static ::std::shared_ptr<Variant<> > _V(V const& v) {
    return ::std::make_shared<Variant<> >(v);
}

typedef ::std::initializer_list<Variant<> const*> args_t;

#define COMXX_PPARGS_0(pre, args)
#define COMXX_PPARGS_1(pre, args) pre(args.begin())
#define COMXX_PPARGS_2(pre, args) COMXX_PPARGS_1(pre, args), pre(args.begin() + 1)
#define COMXX_PPARGS_3(pre, args) COMXX_PPARGS_2(pre, args), pre(args.begin() + 2)
#define COMXX_PPARGS_4(pre, args) COMXX_PPARGS_3(pre, args), pre(args.begin() + 3)
#define COMXX_PPARGS_5(pre, args) COMXX_PPARGS_4(pre, args), pre(args.begin() + 4)
#define COMXX_PPARGS_6(pre, args) COMXX_PPARGS_5(pre, args), pre(args.begin() + 5)
#define COMXX_PPARGS_7(pre, args) COMXX_PPARGS_6(pre, args), pre(args.begin() + 6)
#define COMXX_PPARGS_8(pre, args) COMXX_PPARGS_7(pre, args), pre(args.begin() + 7)
#define COMXX_PPARGS_9(pre, args) COMXX_PPARGS_8(pre, args), pre(args.begin() + 8)

class IObject {
public:
    typedef Variant<> variant_type;

    IObject() : _referencedCount(1) {}

    virtual ~IObject() = default;

    virtual IObject* grab() const;

    virtual bool drop() const;

    virtual variant_type query(IID const &) const = 0;

private:
    mutable ::std::atomic<long> _referencedCount;
};

template<typename T>
class shared_ref {
public:
    shared_ref(T *obj) : _ptr(obj) {
        if (_ptr)
            _ptr = _ptr->grab();
    }

    shared_ref(shared_ref const &r) : _ptr(r._ptr) {
        if (_ptr)
            _ptr = _ptr->grab();
    }

    ~shared_ref() {
        if (_ptr) {
            _ptr->drop();
            _ptr = nullptr;
        }
    }

    inline T *operator->() {
        return _ptr;
    }

    inline T const *operator->() const {
        return _ptr;
    }

private:
    T *_ptr = nullptr;
};

template<typename T, class... Args>
inline shared_ref<T> make_shared_ref(Args &&... args) {
    T *obj = new T(::std::forward<Args>(args)...);
    shared_ref<T> r(obj);
    obj->drop();
    return r;
}

#define COMXX_DEFINE(name) \
    static const COMXX_NS::IID name

#define COMXX_IID(name) \
    const COMXX_NS::IID name

COMXX_DEFINE(IID_NEW) = {0xd0733610, 0x6360, 0x4362, {0xb9, 0x42, 0xf4, 0xdb, 0xd2, 0x25, 0x69, 0xf4}};
COMXX_DEFINE(IID_CLONE) = {0x3e670b35, 0xdd3e, 0x4530, {0xb2, 0xff, 0x77, 0x12, 0xb3, 0x5d, 0xf4, 0x93}};

class CustomObject : public IObject {
public:
    CustomObject() = default;

    virtual ~CustomObject();

    typedef struct {
        ::std::string name;
        variant_type::func_type func;
    } func_t;

    typedef ::std::map<IID, IObject *> objects_t;
    typedef ::std::map<IID, func_t> functions_t;

    virtual variant_type query(IID const &) const;

    virtual void clear();

    virtual void add(IID const &, IObject *);

    virtual void add(IID const &, ::std::string const &name, variant_type::func_type);

    functions_t const &functions() const;

private:
    objects_t _objects;
    functions_t _functions;
};

template<typename T>
inline T grab(T v);

template<>
inline IObject* grab<IObject*>(IObject *v) {
    return v->grab();
}

template<typename T>
inline bool drop(T v);

template<>
inline bool drop<IObject*>(IObject *v) {
    return v->drop();
}

template<typename Types>
inline Variant<Types>::Variant(object_type *v) : vt(VT::OBJECT) {
    _pod.o = v;
    if (_pod.o) {
        _pod.o = grab(_pod.o);
    }
}

template<typename Types>
inline Variant<Types>::Variant() : vt(VT::NIL) {}

template<typename Types>
inline Variant<Types>::Variant(void *v) : vt(VT::POINTER) { _pod.p = v; }

template<typename Types>
inline Variant<Types>::Variant(nullptr_t) : vt(VT::POINTER) { _pod.p = nullptr; }

template<typename Types>
inline Variant<Types>::Variant(int v) : vt(VT::INT) { _pod.i = v; }

template<typename Types>
inline Variant<Types>::Variant(unsigned int v) : vt(VT::UINT) { _pod.ui = v; }

template<typename Types>
inline Variant<Types>::Variant(long v) : vt(VT::LONG) { _pod.l = v; }

template<typename Types>
inline Variant<Types>::Variant(unsigned long v) : vt(VT::ULONG) { _pod.ul = v; }

template<typename Types>
inline Variant<Types>::Variant(short v) : vt(VT::SHORT) { _pod.s = v; }

template<typename Types>
inline Variant<Types>::Variant(unsigned short v) : vt(VT::USHORT) { _pod.us = v; }

template<typename Types>
inline Variant<Types>::Variant(long long v) : vt(VT::LONGLONG) { _pod.ll = v; }

template<typename Types>
inline Variant<Types>::Variant(unsigned long long v) : vt(VT::ULONGLONG) { _pod.ull = v; }

template<typename Types>
inline Variant<Types>::Variant(float v) : vt(VT::FLOAT) { _pod.f = v; }

template<typename Types>
inline Variant<Types>::Variant(double v) : vt(VT::DOUBLE) { _pod.d = v; }

template<typename Types>
inline Variant<Types>::Variant(char v) : vt(VT::CHAR) { _pod.c = v; }

template<typename Types>
inline Variant<Types>::Variant(unsigned char v) : vt(VT::UCHAR) { _pod.uc = v; }

template<typename Types>
inline Variant<Types>::Variant(bool v) : vt(VT::BOOLEAN) { _pod.b = v; }

template<typename Types>
inline Variant<Types>::Variant(bytes_type const &v) : vt(VT::BYTES) { _bytes = ::std::make_shared<bytes_type>(v); }

template<typename Types>
inline Variant<Types>::Variant(::std::string const &v) : vt(VT::STRING) { _str = ::std::make_shared<string_type>(v); }

template<typename Types>
inline Variant<Types>::Variant(char const *v) : vt(VT::STRING) { _str = ::std::make_shared<string_type>(v); }

template<typename Types>
inline Variant<Types>::Variant(func_type v) : vt(VT::FUNCTION) { _pod.fn = v; }

template<typename Types>
inline Variant<Types>::Variant(Variant const &r) : vt(r.vt) {
    _pod = r._pod;
    _bytes = r._bytes;
    _str = r._str;
    if (r.vt == VT::OBJECT && _pod.o) {
        _pod.o = grab(_pod.o);
    }
}

template<typename Types>
inline Variant<Types>::Variant(::std::shared_ptr<Variant> const &r) : vt(r->vt) {
    _pod = r->_pod;
    _bytes = r->_bytes;
    _str = r->_str;
    if (r->vt == VT::OBJECT && _pod.o) {
        _pod.o = grab(_pod.o);
    }
}

template<typename Types>
inline Variant<Types>::~Variant() {
    if (vt == VT::OBJECT && _pod.o) {
        drop(_pod.o);
        _pod.o = nullptr;
    }
}

template <typename TVar = Variant<>, typename TReturn = TVar, typename TArg = TVar const&>
class FunctionTypes
{
public:
    typedef TVar variant_type;
    typedef TReturn return_type;
    typedef TArg arg_type;
};

// 泛函数对象内存单一包裹结构
template <typename Types = FunctionTypes<> >
class Function
{
public:

    typedef typename Types::variant_type variant_type;
    typedef typename Types::return_type return_type;
    typedef typename Types::arg_type arg_type;

    typedef ::std::function<return_type()> fun0_type;
    typedef ::std::function<return_type(arg_type const&)> fun1_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&)> fun2_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&)> fun3_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun4_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun5_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun6_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun7_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun8_type;
    typedef ::std::function<return_type(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&)> fun9_type;

    Function() = default;
    Function(fun0_type fn);
    Function(fun1_type fn);
    Function(fun2_type fn);
    Function(fun3_type fn);
    Function(fun4_type fn);
    Function(fun5_type fn);
    Function(fun6_type fn);
    Function(fun7_type fn);
    Function(fun8_type fn);
    Function(fun9_type fn);

    ~Function();

    return_type operator ()() const;
    return_type operator ()(arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;
    return_type operator ()(arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&, arg_type const&) const;

    operator bool () const;

private:

    unsigned char _count = 0;
    void* _mem = nullptr;
};

template<typename Types>
inline typename Variant<Types>::object_type *Variant<Types>::toObject() const { return _pod.o; }

template<typename Types>
inline void *Variant<Types>::toPointer() const { return _pod.p; }

template<typename Types>
inline int Variant<Types>::toInt() const { return _pod.i; }

template<typename Types>
inline unsigned int Variant<Types>::toUInt() const { return _pod.ui; }

template<typename Types>
inline long Variant<Types>::toLong() const { return _pod.l; }

template<typename Types>
inline unsigned long Variant<Types>::toULong() const { return _pod.ul; }

template<typename Types>
inline short Variant<Types>::toShort() const { return _pod.s; }

template<typename Types>
inline unsigned short Variant<Types>::toUShort() const { return _pod.us; }

template<typename Types>
inline long long Variant<Types>::toLonglong() const { return _pod.ll; }

template<typename Types>
inline unsigned long long Variant<Types>::toULonglong() const { return _pod.ull; }

template<typename Types>
inline float Variant<Types>::toFloat() const { return _pod.f; }

template<typename Types>
inline double Variant<Types>::toDouble() const { return _pod.d; }

template<typename Types>
inline char Variant<Types>::toChar() const { return _pod.c; }

template<typename Types>
inline unsigned char Variant<Types>::toUChar() const { return _pod.uc; }

template<typename Types>
inline bool Variant<Types>::toBool() const { return _pod.b; }

template<typename Types>
inline typename Variant<Types>::bytes_type const &Variant<Types>::toBytes() const { return *_bytes; }

template<typename Types>
inline ::std::string const &Variant<Types>::toString() const { return *_str; }

template<typename Types>
inline typename Variant<Types>::func_type Variant<Types>::toFunction() const { return _pod.fn; }

template<typename Types>
inline Variant<Types> &Variant<Types>::operator=(Variant const &r) {
    if (this != &r) {
        _bytes = r._bytes;
        _str = r._str;

        object_type *old = nullptr;
        if (vt == VT::OBJECT && _pod.o) {
            old = _pod.o;
        }
        const_cast<VT &>(vt) = r.vt;
        _pod = r._pod;
        if (vt == VT::OBJECT && _pod.o) {
            _pod.o = grab(_pod.o);
        }
        if (old) {
            drop(old);
        }
    }
    return *this;
}

inline IObject* IObject::grab() const {
    _referencedCount += 1;
    return const_cast<IObject*>(this);
}

inline bool IObject::drop() const {
    if (--_referencedCount == 0) {
        delete this;
        return true;
    }
    return false;
}

inline CustomObject::variant_type CustomObject::query(IID const &iid) const {
    {
        auto fnd = _objects.find(iid);
        if (fnd != _objects.end())
            return fnd->second;
    }

    {
        auto fnd = _functions.find(iid);
        if (fnd != _functions.end())
            return fnd->second.func;
    }

    return nullptr;
}

inline CustomObject::~CustomObject() {
    for (auto &e : _objects) {
        e.second->drop();
    }
    _objects.clear();
    _functions.clear();
}

inline void CustomObject::clear() {
    for (auto &e : _objects) {
        e.second->drop();
    }
    _objects.clear();
    _functions.clear();
}

inline void CustomObject::add(IID const &iid, IObject *obj) {
    auto fnd = _objects.find(iid);
    if (fnd != _objects.end()) {
        fnd->second->drop();
    }

    _objects[iid] = obj->grab();
}

inline void CustomObject::add(IID const &iid, ::std::string const &name, variant_type::func_type func) {
    _functions[iid] = {name, func};
}

inline CustomObject::functions_t const &CustomObject::functions() const {
    return _functions;
}

template<class T>
struct function_traits {
private:
    using call_type = function_traits<decltype(&T::operator())>;

public:
    using return_type = typename call_type::return_type;
    static const size_t count = call_type::count - 1;

    template<size_t N>
    struct argument {
        static_assert(N < count, "参数数量错误");
        using type = typename call_type::template argument<N + 1>::type;
    };
};

template<class R, class... Args>
struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {
};

template<class R, class... Args>
struct function_traits<R(Args...)> {
    using return_type = R;
    static const size_t count = sizeof...(Args);

    template<size_t N>
    struct argument {
        static_assert(N < count, "参数数量错误");
        using type = typename ::std::tuple_element<N, ::std::tuple<Args...>>::type;
    };
};

template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : public function_traits<R(Args...)> {
};

template<class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> : public function_traits<R(Args...)> {
};

template<class C, class R>
struct function_traits<R(C::*)> : public function_traits<R(C &)> {
};

template<class T>
struct function_traits<T &> : public function_traits<T> {
};

template<class T>
struct function_traits<T &&> : public function_traits<T> {
};

template<typename F, size_t N = function_traits<F>::count>
struct function_call {
    template<typename C, typename Args>
    inline typename function_traits<F>::return_type operator()(F const &fn, C *self, Args const &args) {
        return (self->*fn)();
    }

    template<typename Args>
    inline typename function_traits<F>::return_type operator()(F const &fn, Args const &args) {
        return fn();
    }
};

#define COMXX_FUNCTION_CALL(N, __args__) \
template <typename F> \
struct function_call<F, N> { \
    template <typename C, typename Args> \
    inline typename function_traits<F>::return_type operator()(F const& fn, C* self, Args const& args) { \
        return (self->*fn)(__args__); \
    } \
    template <typename Args> \
    inline typename function_traits<F>::return_type operator()(F const& fn, Args const& args) { \
        return fn(__args__); \
    } \
};

COMXX_FUNCTION_CALL(1, COMXX_PPARGS_1(**, args))
COMXX_FUNCTION_CALL(2, COMXX_PPARGS_2(**, args))
COMXX_FUNCTION_CALL(3, COMXX_PPARGS_3(**, args))
COMXX_FUNCTION_CALL(4, COMXX_PPARGS_4(**, args))
COMXX_FUNCTION_CALL(5, COMXX_PPARGS_5(**, args))
COMXX_FUNCTION_CALL(6, COMXX_PPARGS_6(**, args))
COMXX_FUNCTION_CALL(7, COMXX_PPARGS_7(**, args))
COMXX_FUNCTION_CALL(8, COMXX_PPARGS_8(**, args))
COMXX_FUNCTION_CALL(9, COMXX_PPARGS_9(**, args))


#define COMXX_CUSTOMOBJECT_ADD(idd, name, func) \
add(idd, #name, [&](COMXX_NS::args_t const& args)->COMXX_NS::Variant { \
return COMXX_NS::function_call<decltype(&func)>()(&func, this, args); \
    })

#define _COMXX_FUNCTION_IMPL(num) \
template <typename Types> \
inline Function<Types>::Function(fun##num##_type fn) \
: _count(num), _mem(new fun##num##_type(fn)) \
{}

_COMXX_FUNCTION_IMPL(0)
_COMXX_FUNCTION_IMPL(1)
_COMXX_FUNCTION_IMPL(2)
_COMXX_FUNCTION_IMPL(3)
_COMXX_FUNCTION_IMPL(4)
_COMXX_FUNCTION_IMPL(5)
_COMXX_FUNCTION_IMPL(6)
_COMXX_FUNCTION_IMPL(7)
_COMXX_FUNCTION_IMPL(8)
_COMXX_FUNCTION_IMPL(9)

template <typename Types>
inline Function<Types>::~Function()
{
#define _COMXX_FUNCTION_FREE(num) \
case num: delete (fun##num##_type*)_mem; break;
    switch (_count) {
        _COMXX_FUNCTION_FREE(0)
        _COMXX_FUNCTION_FREE(1)
        _COMXX_FUNCTION_FREE(2)
        _COMXX_FUNCTION_FREE(3)
        _COMXX_FUNCTION_FREE(4)
        _COMXX_FUNCTION_FREE(5)
        _COMXX_FUNCTION_FREE(6)
        _COMXX_FUNCTION_FREE(7)
        _COMXX_FUNCTION_FREE(8)
        _COMXX_FUNCTION_FREE(9)
    }
}

template <typename Types>
inline Function<Types>::operator bool() const {
    if (!_mem)
        return false;
#define _COMXX_FUNCTION_NOTEMPTY(num) \
        case num: return !!*((fun##num##_type*)_mem);
    switch (_count) {
        _COMXX_FUNCTION_NOTEMPTY(0)
        _COMXX_FUNCTION_NOTEMPTY(1)
        _COMXX_FUNCTION_NOTEMPTY(2)
        _COMXX_FUNCTION_NOTEMPTY(3)
        _COMXX_FUNCTION_NOTEMPTY(4)
        _COMXX_FUNCTION_NOTEMPTY(5)
        _COMXX_FUNCTION_NOTEMPTY(6)
        _COMXX_FUNCTION_NOTEMPTY(7)
        _COMXX_FUNCTION_NOTEMPTY(8)
        _COMXX_FUNCTION_NOTEMPTY(9)
    }
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()() const {
    assert(_count == 0);
    return (*(fun0_type*)_mem)();
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0) const {
    assert(_count == 1);
    return (*(fun1_type*)_mem)(v0);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1) const {
    assert(_count == 2);
    return (*(fun2_type*)_mem)(v0, v1);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2) const {
    assert(_count == 3);
    return (*(fun3_type*)_mem)(v0, v1, v2);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3) const {
    assert(_count == 4);
    return (*(fun4_type*)_mem)(v0, v1, v2, v3);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3, arg_type const& v4) const {
    assert(_count == 5);
    return (*(fun5_type*)_mem)(v0, v1, v2, v3, v4);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3, arg_type const& v4, arg_type const& v5) const {
    assert(_count == 6);
    return (*(fun6_type*)_mem)(v0, v1, v2, v3, v4, v5);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3, arg_type const& v4, arg_type const& v5, arg_type const& v6) const {
    assert(_count == 7);
    return (*(fun7_type*)_mem)(v0, v1, v2, v3, v4, v5, v6);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3, arg_type const& v4, arg_type const& v5, arg_type const& v6, arg_type const& v7) const {
    assert(_count == 8);
    return (*(fun8_type*)_mem)(v0, v1, v2, v3, v4, v5, v6, v7);
}

template <typename Types>
inline typename Function<Types>::return_type Function<Types>::operator()(arg_type const& v0, arg_type const& v1, arg_type const& v2, arg_type const& v3, arg_type const& v4, arg_type const& v5, arg_type const& v6, arg_type const& v7, arg_type const& v8) const {
    assert(_count == 9);
    return (*(fun9_type*)_mem)(v0, v1, v2, v3, v4, v5, v6, v7, v8);
}

COMXX_END

#endif
