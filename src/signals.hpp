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

    /** 是否请求中断了emit过程 */
    bool veto;

    /** 附加数据 */
    ::std::shared_ptr<::COMXX_NS::Variant<> > payload;
};

// 插槽对象
class Slot {
public:

    typedef void (*pfn_callback_type)(Slot &);

    typedef void (Object::*pfn_membercallback_type)(Slot &);

    // 基于function对象实现的slot不能disconnect和查询有无连接，受制于stl所限
    typedef ::std::function<void(Slot &)> callback_type;

    typedef ::std::shared_ptr<Tunnel> tunnel_type;
    typedef ::std::shared_ptr<::COMXX_NS::Variant<> > payload_type;
    typedef payload_type data_type;

    /** 重定向的信号 */
    signal_t redirect;

    // 通用回调对象
    callback_type cb;

    /** 回调的上下文 */
    attach_ptr<Object> target;

    /** 激发者 */
    attach_ptr<Object> sender;

    /** 数据 */
    data_type data;

    /** 延迟s启动 */
    // seconds_t delay = 0;

    /** 穿透用的数据 */
    tunnel_type tunnel;

    /** connect 时附加的数据 */
    payload_type payload;

    /** 信号源 */
    signal_t signal;

    /** 激发频率限制 (emits per second) */
    unsigned short eps = 0;

    /** 是否中断掉信号调用树 */
    bool getVeto() const;

    void setVeto(bool b);

    /** 调用几次自动解绑，默认为 null，不使用概设定 */
    size_t count = 0;
    size_t emitedCount = 0;

    /** 激发信号
     @data 附带的数据，激发后自动解除引用 */
    void emit(data_type data, tunnel_type tunnel);

protected:

    void _doEmit();

    //  函数回调
    pfn_callback_type _pfn_cb = nullptr;

    // 对象的函数回调
    pfn_membercallback_type _pfn_memcb = nullptr;

private:

    double _epstm = 0;
    bool _veto;

    friend class Slots;

    friend class Signals;
};

class Slots {
public:

    typedef ::std::shared_ptr<Slot> slot_type;

    ~Slots();

    // 所有者，会传递到 Slot 的 sender
    attach_ptr<Object> owner;

    // 信号源
    signal_t signal;

    /** 清空连接 */
    void clear();

    void block();

    void unblock();

    /** 是否已经阻塞 */
    bool isblocked() const;

    /** 添加一个插槽 */
    void add(slot_type s);

    /** 对所有插槽激发信号
     * @note 返回被移除的插槽的对象
     */
    ::std::set<Object *> emit(Slot::data_type data, Slot::tunnel_type tunnel);

    bool disconnect(Slot::pfn_callback_type cb);

    bool disconnect(Slot::pfn_membercallback_type cb, Object *target);

    slot_type find_connected_function(Slot::pfn_callback_type cb);

    slot_type find_connected_function(Slot::pfn_membercallback_type cb, Object *target);

    slot_type find_redirected(signal_t const &sig, Object *target);

    bool is_connected(Object *target) const;

private:

    typedef ::std::vector<slot_type> slots_type;

    // 保存所有插槽
    slots_type _slots;

    /** 阻塞信号
     * @note emit被阻塞的信号将不会有任何作用
     */
    int _blk = 0;

    friend class Signals;
};

class Signals {
public:

    typedef ::std::shared_ptr<Slots> slots_type;

    explicit Signals(Object* owner)
        : owner(owner)
    {
        // pass
    }
    
    ~Signals();

    // 清空
    void clear();

    /** 注册信号 */
    bool registerr(signal_t const &sig);

    // 信号的主体
    attach_ptr<Object> owner;

    /** 只连接一次 */
    Slots::slot_type once(signal_t const &sig, Slot::callback_type cb);

    Slots::slot_type once(signal_t const &sig, Slot::pfn_callback_type cb);

    Slots::slot_type once(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    template<typename C>
    Slots::slot_type once(signal_t const &sig, void (C::*cb)(Slot &), C *target);

    /** 连接信号插槽 */
    Slots::slot_type connect(signal_t const &sig, Slot::callback_type cb);

    Slots::slot_type connect(signal_t const &sig, Slot::pfn_callback_type cb);

    Slots::slot_type connect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    template<typename C>
    Slots::slot_type connect(signal_t const &sig, void (C::*cb)(Slot &), C *target);

    /** 该信号是否存在连接上的插槽 */
    bool isConnected(signal_t const &sig) const;

    /** 转发一个信号到另一个对象的信号 */
    Slots::slot_type redirect(signal_t const &sig1, signal_t const &sig2, Object *target);

    Slots::slot_type redirect(signal_t const &sig, Object *target);

    /** 激发信号 */
    void emit(signal_t const& sig, Slot::data_type data = nullptr, Slot::tunnel_type tunnel = nullptr) const;

    /** 向外抛出信号
     * @note 为了解决诸如金币变更、元宝变更等大部分同时发生的事件但是因为set的时候不能把这些的修改函数合并成1个函数处理，造成同一个消息短时间多次激活，所以使用该函数可以在下一帧开始后归并发出唯一的事件。所以该函数出了信号外不支持其他带参
     */
    // void cast(signal_t const &sig);

    /** 断开连接 */
    void disconnectOfTarget(Object *target, bool inv = true);

    void disconnect(signal_t const &sig);

    void disconnect(signal_t const &sig, Slot::pfn_callback_type cb);

    void disconnect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target);

    bool isConnectedOfTarget(Object *target) const;

    /** 阻塞一个信号，将不响应激发 */
    void block(signal_t const &sig);

    void unblock(signal_t const &sig);

    bool isblocked(signal_t const &sig) const;

protected:

    // void _doCastings();

    Slots::slot_type connect(signal_t const &sig, Slot::callback_type cb, Object *target, Slot::pfn_membercallback_type cbmem);

private:

    void _inv_connect(Object *target);

    void _inv_disconnect(Object* target) const;

    // 反向登记，当自身 dispose 时，需要和对方断开
    ::std::set<Signals *> _invs;

    typedef ::std::map<signal_t, slots_type> signals_type;
    signals_type _signals;

    // ::std::set<signal_t> _castings;
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
    return connect(sig, ::std::bind(cb, target, ::std::placeholders::_1), target, (Slot::pfn_membercallback_type)cb);
}

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
    ::std::shared_ptr<Signals> _s;
    friend class Signals;
};

#define SS_SIGNAL(sig) static const ::SS_NS::signal_t sig;
#define SS_SIGNAL_IMPL(sig, val) const ::SS_NS::signal_t sig = val;

SS_END
