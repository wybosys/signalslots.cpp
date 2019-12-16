#pragma once

// github.com/wybosys/cppsignals.git

#define SS_BEGIN namespace Ss {
#define SS_END }
#define USE_SS using namespace Ss;

#include <memory>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>

SS_BEGIN

#define SS_LOG_WARN(msg) ::std::cerr << msg << ::std::endl;
#define SS_LOG_INFO(msg) ::std::cout << msg << ::std::endl;
#define SS_LOG_FATAL(msg) { ::std::cerr << msg << ::std::endl; throw msg; }

typedef double seconds_t;
typedef ::std::string signal_t;

class Object;

class SObject;

class Slot;

class Slots;

class Signals;

template<typename T>
class ReferenceOrPointer {
public:
    ReferenceOrPointer(T *v) : _ptr(v) {}

    ReferenceOrPointer(T &v) : _ptr(&v) {}

    inline T *operator->() {
        return _ptr;
    }

    inline T const &operator->() const {
        return _ptr;
    }

    inline operator T &() {
        return *_ptr;
    }

    inline operator T const &() const {
        return *_ptr;
    }

private:
    T *_ptr;
};

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

struct IPayload {
    virtual ~IPayload() = default;
};

// 用于穿透整个emit流程的对象
struct Tunnel {

    /** 是否请求中断了emit过程 */
    bool veto;

    /** 附加数据 */
    ::std::shared_ptr<IPayload> payload;
};

// 插槽对象
class Slot {
public:

    typedef void (*raw_callback_type)();

    typedef void (*callback_type)(Slot const &s);

    typedef void (Object::*object_callback_type)(Slot const &s);

    typedef ::std::shared_ptr<Tunnel> tunnel_type;
    typedef ::std::shared_ptr<IPayload> payload_type;
    typedef payload_type data_type;

    /** 重定向的信号 */
    signal_t redirect;

    /** 回调 */
    raw_callback_type cb = nullptr;

    /** 回调的上下文 */
    attach_ptr<SObject> target;

    /** 激发者 */
    attach_ptr<SObject> sender;

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
    ushort eps = 0;

    /** 是否中断掉信号调用树 */
    [[nodiscard]]
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

private:

    seconds_t __epstm = 0;
    bool __veto;
};

class Slots {
public:

    typedef ::std::shared_ptr<Slot> slot_type;

    ~Slots();

    // 所有者，会传递到 Slot 的 sender
    attach_ptr<SObject> owner;

    // 信号源
    signal_t signal;

    /** 清空连接 */
    void clear();

    void block();

    void unblock();

    /** 是否已经阻塞 */
    [[nodiscard]]
    bool isblocked() const;

    /** 添加一个插槽 */
    void add(slot_type s);

    /** 对所有插槽激发信号
     * @note 返回被移除的插槽的对象
     */
    ::std::set<SObject *> emit(Slot::data_type data, Slot::tunnel_type tunnel);

    bool disconnect(Slot::raw_callback_type cb, SObject *target);

    slot_type find_connected_function(Slot::raw_callback_type cb, SObject *target);

    slot_type find_redirected(signal_t const &sig, SObject *target);

    bool is_connected(SObject *target) const;

private:

    typedef ::std::vector<slot_type> __slots_type;

    // 保存所有插槽
    __slots_type __slots;

    /** 阻塞信号
     * @note emit被阻塞的信号将不会有任何作用
     */
    uint __block = 0;

    friend class Signals;
};

class Signals {
public:

    typedef ::std::shared_ptr<Slots> slots_type;

    explicit Signals(SObject &owner) : owner(owner) {}

    ~Signals();

    // 清空
    void clear();

    /** 注册信号 */
    bool registerr(signal_t const &sig);

    // 信号的主体
    SObject &owner;

    /** 只连接一次 */
    Slots::slot_type once(signal_t const &sig, Slot::raw_callback_type cb, SObject *target = nullptr);

    /** 连接信号插槽 */
    Slots::slot_type connect(signal_t const &sig, Slot::raw_callback_type cb, SObject *target = nullptr);

    /** 该信号是否存在连接上的插槽 */
    [[nodiscard]]
    bool isConnected(signal_t const &sig) const;

    /** 转发一个信号到另一个对象的信号 */
    Slots::slot_type redirect(signal_t const &sig1, signal_t const &sig2, SObject *target);

    Slots::slot_type redirect(signal_t const &sig, SObject *target);

    /** 激发信号 */
    void emit(signal_t const &sig, Slot::data_type data = nullptr, Slot::tunnel_type tunnel = nullptr);

    /** 向外抛出信号
     * @note 为了解决诸如金币变更、元宝变更等大部分同时发生的事件但是因为set的时候不能把这些的修改函数合并成1个函数处理，造成同一个消息短时间多次激活，所以使用该函数可以在下一帧开始后归并发出唯一的事件。所以该函数出了信号外不支持其他带参
     */
    // void cast(signal_t const &sig);

    /** 断开连接 */
    void disconnectOfTarget(SObject *target, bool inv = true);

    void disconnect(signal_t const &sig, Slot::raw_callback_type cb = nullptr, SObject *target = nullptr);

    bool isConnectedOfTarget(SObject *target) const;

    /** 阻塞一个信号，将不响应激发 */
    void block(signal_t const &sig);

    void unblock(signal_t const &sig);

    [[nodiscard]]
    bool isblocked(signal_t const &sig) const;

protected:

    // void _doCastings();

private:

    void __inv_connect(SObject *target);

    void __inv_disconnect(SObject *target);

    // 反向登记，当自身 dispose 时，需要和对方断开
    ::std::set<Signals *> __invs;

    typedef ::std::map<signal_t, slots_type> __slots_type;
    __slots_type __slots;

    // ::std::set<signal_t> __castings;
};

class Object {
public:
    virtual ~Object() = default;
};

class SObject : public Object {
public:

    SObject() : _s(*this) {}

    inline Signals &signals() {
        return _s;
    }

private:
    Signals _s;
};

SS_END
