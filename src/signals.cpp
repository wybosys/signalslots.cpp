#include "signals.hpp"

#include <algorithm>
#include <chrono>
#include <future>
#include <utility>

SS_BEGIN

// 获得当前时间
static double TimeCurrent() {
    auto now = ::std::chrono::system_clock::now().time_since_epoch();
    return ::std::chrono::duration_cast<::std::chrono::microseconds>(now).count() / 1000000.;
}

// ------------------------------------- slot

Slot::Slot()
{
    // pass
}

Slot::~Slot()
{
    // pass
}

bool Slot::getVeto() const {
    return _veto;
}

void Slot::setVeto(bool b) {
    _veto = b;
    if (tunnel)
        tunnel->veto = b;
}

void Slot::emit(Slot::data_type d, Slot::tunnel_type t) {
    if (eps) {
        double now = TimeCurrent();
        if (_epstm == 0) {
            _epstm = now;
        } else {
            double el = now - _epstm;
            //this._epstms = now; 注释以支持快速多次点击中可以按照频率命中一次，而不是全部都忽略掉
            if ((1000 / el) > eps)
                return;
            _epstm = now; //命中一次后重置时间
        }
    }

    this->data = ::std::move(d);
    this->tunnel = ::std::move(t);

    _doEmit();
}

void Slot::_doEmit() {
    // 成员函数指针会将target通过bind到函数对象中，所以不需要采用传统调用成员指针的方法调用
    // 普通函数指针可以直接调用
    cb(*this);

    // 清理
    data = nullptr;
    tunnel = nullptr;

    // 增加计数
    ++emitedCount;
}

// --------------------------------------- slots

Slots::Slots()
{
    // pass
}

Slots::~Slots() {
    clear();
}

void Slots::clear() {
    _slots.clear();
}

void Slots::block() {
    ++_blk;
}

void Slots::unblock() {
    --_blk;
}

bool Slots::isblocked() const {
    return _blk != 0;
}

void Slots::add(Slots::slot_type s) {
    _slots.emplace_back(s);
}

::std::set<Object *> Slots::emit(Slot::data_type d, Slot::tunnel_type t) {
    ::std::set<Object *> r;
    if (isblocked())
        return r;

    // 保存一份快照
    // 如果循环中存在对slots的修改，则采用:
    // 1, 总循环和emit使用快照
    // 2, 删除使用查找-》删除

    slots_type snaps = _slots;
    for (size_t idx = 0; idx < snaps.size(); ++idx)
    {
        auto &s = snaps[idx];
        if (s->count && (s->emitedCount >= s->count))
            continue; // 已经达到设置激活的数量
        
        // 激发信号
        s->signal = signal;
        s->sender = owner;
        s->emit(d, t);

        // 判断激活数是否达到设置
        if (s->count && s->emitedCount >= s->count) {
            // 需要移除
            if (s->target)
                r.insert(s->target);
            // 在_slots中查找删除
            for (auto iter = _slots.begin(); iter != _slots.end(); ++iter)
            {
                if (*iter == s) {
                    iter = _slots.erase(iter);
                    break; // 找到即删除
                }
            }
        }

        // 阻断，停止执行
        if (s->getVeto()) {
            break;
        }

        if (!_signals->owner) {
            // 如果运行过程中根对象已经析构，则停止执行
            break;
        }
    }

    if (!_signals->owner) {
        // 返回空的列表，因为对象已经析构，会自动断开其他连接, 返回运行中断开的对象列表已经没有意义
        r.clear();
    }

    return r;
}

Slots::slot_type Signals::once(signal_t const &sig, Slot::callback_type cb) {
    auto r = connect(sig, cb);
    if (r)
        r->count = 1;
    return r;
}

Slots::slot_type Signals::once(signal_t const &sig, Slot::pfn_callback_type cb) {
    auto r = connect(sig, cb);
    if (r)
        r->count = 1;
    return r;
}

Slots::slot_type Signals::once(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target) {
    auto r = connect(sig, cb, target);
    if (r)
        r->count = 1;
    return r;
}

bool Slots::disconnect(Slot::pfn_callback_type cb) {
    bool r = false;
    for (auto iter = _slots.begin(); iter != _slots.end();) {
        auto s = *iter;

        if (s->_pfn_cb == cb) {
            r = true;
            iter = _slots.erase(iter);
        } else {
            ++iter;
        }
    }
    return r;
}

bool Slots::disconnect(Slot::pfn_membercallback_type cb, Object *target) {
    bool r = false;
    for (auto iter = _slots.begin(); iter != _slots.end();) {
        auto s = *iter;

        if (cb && s->_pfn_memcb != cb) {
            ++iter;
            continue;
        }

        if (s->target == target) {
            r = true;
            iter = _slots.erase(iter);
        } else {
            ++iter;
        }
    }
    return r;
}

Slots::slot_type Slots::findByFunction(Slot::pfn_callback_type cb) const {
    for (auto iter = _slots.begin(); iter != _slots.end(); ++iter) {
        auto &s = *iter;
        if (s->_pfn_cb == cb) {
            return s;
        }
    }
    return nullptr;
}

Slots::slot_type Slots::findByFunction(Slot::pfn_membercallback_type cb, Object *target) const {
    for (auto iter = _slots.begin(); iter != _slots.end(); ++iter) {
        auto &s = *iter;
        if (s->_pfn_memcb == cb && s->target == target) {
            return s;
        }
    }
    return nullptr;
}

bool Slots::isConnected(Object *target) const {
    for (auto iter = _slots.begin(); iter != _slots.end(); ++iter) {
        auto &s = *iter;
        if (s->target == target) {
            return true;
        }
    }
    return false;
}

size_t Slots::size() const {
    return _slots.size();
}

// ---------------------------------------- signals

Signals::Signals(Object* _owner)
    : owner(_owner)
{
    // pass
}

Signals::~Signals() {
    clear();
    owner = nullptr;
}

void Signals::clear() {
    // 清空反向的连接
    auto snaps = _inverses;
    for (auto &iter: snaps) {
        if (iter->owner && iter->owner != owner) {
            iter->owner->signals().disconnectOfTarget(owner);
        }
    }
    _inverses.clear();

    // 清空slot的连接
    for (auto &iter: _signals) {
        auto& ss = iter.second;
        for (auto& iter : ss->_slots) {
            if (iter->target && iter->target != owner) {
                disconnect(iter->signal, iter->_pfn_memcb, iter->target);
            }
        }
        ss->clear();
    }
    _signals.clear();
}

bool Signals::registerr(signal_t const &sig) {
    if (sig.empty()) {
        SS_LOG_WARN("不能注册一个空信号")
        return false;
    }

    if (_signals.find(sig) != _signals.end())
        return false;

    auto ss = ::std::make_shared<Slots>();
    ss->_signals = this;
    ss->signal = sig;
    ss->owner = owner;
    _signals.insert(::std::make_pair(sig, ss));
    return true;
}

Signals::slots_type Signals::find(signal_t const& s) const {
    auto fnd = _signals.find(s);
    return fnd == _signals.end() ? nullptr : fnd->second;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::pfn_callback_type cb) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->findByFunction(cb);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_pfn_cb = cb;
    s->cb = cb;
    ss->add(s);

    return s;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::callback_type cb) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;
    auto s = ::std::make_shared<Slot>();

    s->cb = std::move(cb);
    ss->add(s);

    return s;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->findByFunction(cb, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_pfn_memcb = cb;
    s->cb = ::std::bind(cb, target, ::std::placeholders::_1);
    s->target = target;
    ss->add(s);

    // 将自己添加到连接目标的反向连接中，当对方析构时，对方会使用反向连接自动断开和当前的连接
    // 如果连接的是自己，则不连接
    if (target != owner) {
        target->_s->_inverses.insert(this);
    }

    return s;
}

Slots::slot_type Signals::_connect(signal_t const &sig, Slot::callback_type cb, Object *target, Slot::pfn_membercallback_type cbmem) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->findByFunction(cbmem, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_pfn_memcb = cbmem;
    s->cb = ::std::move(cb);
    s->target = target;
    ss->add(s);

    if (target != owner) {
        target->_s->_inverses.insert(this);
    }

    return s;
}

bool Signals::isConnected(signal_t const &sig) const {
    auto fnd = _signals.find(sig);
    return fnd != _signals.end() && !fnd->second->_slots.empty();
}

void Signals::emit(signal_t const &sig, Slot::data_type d, Slot::tunnel_type t) const {
    // 保护signals，避免运行期被释放
    ::std::shared_ptr<Signals> lifekeep(owner->_s);

    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return;
    }

    // 使用快照避免owner析构 -> signals::clear -> 导致slots被释放
    auto snaps = fnd->second;
    auto targets = snaps->emit(d, t);
    if (!targets.empty()) {
        // 收集所有被移除的target，并断开反向连接
        for (auto &iter: targets) {
            if (!isConnectedOfTarget(iter)) {
                iter->_s->_inverses.erase(const_cast<Signals*>(this));
            }
        }
    }
}

void Signals::disconnectOfTarget(Object *target) {
    if (target == nullptr)
        return;

    for (auto &iter : _signals) {
        iter.second->disconnect(nullptr, target);
    }

    if (target != owner) {
        if (!isConnectedOfTarget(target)) {
            target->_s->_inverses.erase(const_cast<Signals*>(this));
        }
    }
}

void Signals::disconnect(signal_t const &sig) {
    disconnect(sig, nullptr);
}

void Signals::disconnect(signal_t const &sig, Slot::pfn_callback_type cb) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end())
        return;
    auto &ss = fnd->second;

    if (cb == nullptr) {
        // 清除sig的所有插槽，自动断开反向引用
        ::std::set<Object *> targets;
        for (const auto &iter:ss->_slots) {
            if (iter->target)
                targets.insert(iter->target);
        }
        ss->_slots.clear();
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter)) {
                iter->_s->_inverses.erase(const_cast<Signals*>(this));
            }
        }
    } else {
        ss->disconnect(cb);
    }
}

void Signals::disconnect(signal_t const &sig, Slot::pfn_membercallback_type cb, Object *target) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end())
        return;
    auto &ss = fnd->second;

    if (cb == nullptr && target == nullptr) {
        // 清除sig的所有插槽，自动断开反向引用
        ::std::set<Object *> targets;
        for (const auto &iter:ss->_slots) {
            if (iter->target)
                targets.insert(iter->target);
        }
        ss->_slots.clear();
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter)) {
                iter->_s->_inverses.erase(const_cast<Signals*>(this));
            }
        }
    } else {
        // 先清除对应的slot，再判断是否存在和target相连的插槽，如过不存在，则断开反向连接
        if (ss->disconnect(cb, target) && target && !isConnectedOfTarget(target)) {
            target->_s->_inverses.erase(const_cast<Signals*>(this));
        }
    }
}

bool Signals::isConnectedOfTarget(Object *target) const {
    for (auto iter = _signals.cbegin(); iter != _signals.end(); ++iter) {
        if (iter->second->isConnected(target)) {
            return true;
        }
    }
    return false;
}

void Signals::block(signal_t const &sig) {
    auto fnd = _signals.find(sig);
    if (fnd != _signals.end())
        fnd->second->block();
}

void Signals::unblock(signal_t const &sig) {
    auto fnd = _signals.find(sig);
    if (fnd != _signals.end())
        fnd->second->unblock();
}

bool Signals::isblocked(signal_t const &sig) const {
    auto fnd = _signals.find(sig);
    return fnd != _signals.end() ? fnd->second->isblocked() : false;
}

SS_END