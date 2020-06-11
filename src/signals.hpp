#pragma once

// github.com/wybosys/cppsignals.git

#include "com++.hpp"

#define SS_NS ss
#define SS_BEGIN namespace SS_NS {
#define SS_END }
#define USE_SS using namespace SS_NS;

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <functional>
#include <atomic>

SS_BEGIN

#define SS_LOG_WARN(msg) ::std::cerr << (msg) << ::std::endl;
#define SS_LOG_INFO(msg) ::std::cout << (msg) << ::std::endl;
#define SS_LOG_FATAL(msg) { ::std::cerr << (msg) << ::std::endl; throw msg; }

typedef ::std::string signal_t;

class Object;

class Slot;

class Slots;

class Signals;

template<typename T>
class attach_ptr {
public:

    attach_ptr(T *p = nullptr) : _ptr(p) {}

    inline operator T *() {
        return _ptr;
    }

    inline operator T const *() const {
        return _ptr;
    }

    inline T *operator->() {
        return _ptr;
    }

    inline T const *operator->() const {
        return _ptr;
    }

    inline T *ptr() {
        return _ptr;
    }

    inline T const *ptr() const {
        return _ptr;
    }

private:
    T *_ptr;
};

// 用于穿透整个emit流程的对象
struct Tunnel {

    // 是否请求中断了emit过程
    bool veto;

    // 附加数据
    ::std::shared_ptr<::COMXX_NS::Variant<> > payload;
};

// 插槽对象
class Slot {
public:

    Slot();
    ~Slot();

    typedef void (*pfn_callback_type)(Slot &);

    typedef void (Object::*pfn_membercallback_type)(Slot &);

    // 基于function对象实现的slot不能disconnect和查询有无连接，受制于stl所限
    typedef ::std::function<void(Slot &)> callback_type;

    typedef ::std::shared_ptr<Tunnel> tunnel_type;
    typedef ::std::shared_ptr<::COMXX_NS::Variant<> > payload_type;
    typedef payload_type data_type;

    // 通用回调对象
    callback_type cb;

    // 回调函数归属的对象
    attach_ptr<Object> target;

    // 激发对象
    attach_ptr<Object> sender;

    // 携带数据
    data_type data;

    // 穿透整个调用流程的数据
    tunnel_type tunnel;

    // connect 时附加的数据
    payload_type payload;

    // 信号源名称
    signal_t signal;

    // 激发频率限制 (emits per second)
    unsigned short eps = 0;

    // 是否中断掉信号调用树
    bool getVeto() const;

    // 设置中断信号调用
    void setVeto(bool b);

    // 调用几次自动解绑，默认为 null，不使用概设定
    size_t count = 0;
    size_t emitedCount = 0;

    // 激发信号 @data 附带的数据，激发后自动解除引用
    void emit(data_type data, tunnel_type tunnel);

protected:

    void _doEmit();

    //  函数回调
    pfn_callback_type _pfn_cb = nullptr;

    // 对象的函数回调
    pfn_membercallback_type _pfn_memcb = nullptr;

private:

    double _epstm = 0;
    bool _veto = false;

    friend class Slots;
    friend class Signals;
};

// 插槽集合
class Slots {
public:

    Slots();
    ~Slots();

    typedef ::std::shared_ptr<Slot> slot_type;

    // 所有者，会传递到 Slot 的 sender
    attach_ptr<Object> owner;

    // 信号源名称
    signal_t signal;

    // 清空连接
    void clear();

    // 阻塞所有插槽
    void block();

    // 解阻
    void unblock();

    // 是否已经阻塞
    bool isblocked() const;

    // 添加一个插槽
    void add(slot_type s);

    // 对所有插槽激发信号 @note 返回被移除的插槽的对象
    ::std::set<Object *> emit(Slot::data_type data, Slot::tunnel_type tunnel);

    // 移除
    bool disconnect(Slot::pfn_callback_type cb);

    // 移除
    bool disconnect(Slot::pfn_membercallback_type cb, Object *target);

    // 查找插槽
    slot_type findByFunction(Slot::pfn_callback_type cb) const;

    // 查找插槽
    slot_type findByFunction(Slot::pfn_membercallback_type cb, Object *target) const;

    // 是否已经连接
    bool isConnected(Object *target) const;

    // 已经连接的插槽数量
    size_t size() const;

private:

    typedef ::std::vector<slot_type> slots_type;

    // 保存所有插槽
    slots_type _slots;

    // 阻塞信号计数器 @note emit被阻塞的信号将不会有任何作用
    int _blk = 0;

    // 隶属的signals
    attach_ptr<Signals> _signals;

    friend class Signals;
};

// 信号主类
class Signals {
public:

    // 信号必须依赖一个object，使得插槽可以实现为成员函数
    explicit Signals(Object* owner);
    
    ~Signals();

    typedef ::std::shared_ptr<Slots> slots_type;

    // 清空
    void clear();

    // 注册信号
    bool registerr(signal_t const &sig);

    // 返回指定信号的所有插槽
    slots_type find(signal_t const&) const;

    // 信号的主体
    attach_ptr<Object> owner;

    // 只连接一次，调用后自动断开
    Slots::slot_type once(signal_t const &sig, Slot::callback_type cb);

    Slots::slot_type once(signal_t const &sig, Slot::pfn_callback_type cb);

    Slots::slot_type once(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    template<typename C>
    Slots::slot_type once(signal_t const &sig, void (C::*cb)(Slot &), C *target);

    // 连接信号插槽
    Slots::slot_type connect(signal_t const &sig, Slot::callback_type cb);

    Slots::slot_type connect(signal_t const &sig, Slot::pfn_callback_type cb);

    Slots::slot_type connect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    template<typename C>
    Slots::slot_type connect(signal_t const &sig, void (C::*cb)(Slot &), C *target);

    // 该信号是否存在连接上的插槽
    bool isConnected(signal_t const &sig) const;

    // 激发信号
    void emit(signal_t const& sig, Slot::data_type data = nullptr, Slot::tunnel_type tunnel = nullptr) const;

    // 断开连接
    void disconnectOfTarget(Object *target, bool inv = true);

    void disconnect(signal_t const &sig);

    void disconnect(signal_t const &sig, Slot::pfn_callback_type cb);

    void disconnect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    bool isConnectedOfTarget(Object *target) const;

    // 阻塞一个信号，将不响应激发
    void block(signal_t const &sig);

    // 解阻
    void unblock(signal_t const &sig);

    // 是否阻塞
    bool isblocked(signal_t const &sig) const;

protected:

    // 实现连接
    Slots::slot_type _connect(signal_t const &sig, Slot::callback_type cb, Object *target, Slot::pfn_membercallback_type cbmem);

private:

    // 反向登记，当自身 dispose 时，需要和对方断开
    void _inverseConnect(Object *target);

    void _inverseDisconnect(Object* target) const;

    // 保存连接到自身信号的对象信号，用于反向断开
    ::std::set<Signals *> _invs;

    // 保存所有的信号和插槽列表
    typedef ::std::map<signal_t, slots_type> signals_type;
    signals_type _signals;
};

template<typename C>
inline Slots::slot_type Signals::once(signal_t const &sig, void (C::*cb)(Slot &), C *target) {
    auto r = connect(sig, cb, target);
    if (r)
        r->count = 1;
    return r;
}

template<typename C>
inline Slots::slot_type Signals::connect(signal_t const &sig, void (C::*cb)(Slot &), C *target) {
    return _connect(sig, ::std::bind(cb, target, ::std::placeholders::_1), target, (Slot::pfn_membercallback_type)cb);
}

// 基础对象，用于实现成员函数插槽
class Object {
public:

    Object() 
        :_s(::std::make_shared<Signals>(this))
    {
        // pass
    }

    virtual ~Object() {
        if (_s) {
            _s->clear();
            _s->owner = nullptr;
            _s = nullptr;
        }
    }

    inline Signals &signals() {
        return *_s;
    }

    inline Signals const& signals() const {
        return *_s;
    }

private:

    // 实现为shared指针，以用于emit时可以临时保护，避免直接被释放导致野指针
    ::std::shared_ptr<Signals> _s;
    friend class Signals;
};

#define SS_SIGNAL(sig) static const ::SS_NS::signal_t sig;
#define SS_SIGNAL_IMPL(sig, val) const ::SS_NS::signal_t sig = val;

SS_END
