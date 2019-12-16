#include "signals.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <utility>

SS_BEGIN

// 获得当前时间
static seconds_t TimeCurrent() {
    auto now = ::std::chrono::system_clock::now().time_since_epoch();
    return ::std::chrono::duration_cast<::std::chrono::microseconds>(now).count() / 1000000.;
}

// ------------------------------------- slot

bool Slot::getVeto() const {
    return __veto;
}

void Slot::setVeto(bool b) {
    __veto = b;
    if (tunnel)
        tunnel->veto = b;
}

void Slot::emit(Slot::data_type d, Slot::tunnel_type t) {
    if (eps) {
        seconds_t now = TimeCurrent();
        if (__epstm == 0) {
            __epstm = now;
        } else {
            seconds_t el = now - __epstm;
            //this._epstms = now; 注释以支持快速多次点击中可以按照频率命中一次，而不是全部都忽略掉
            if ((1000 / el) > eps)
                return;
            __epstm = now; //命中一次后重置时间
        }
    }

    this->data = ::std::move(d);
    this->tunnel = ::std::move(t);

    _doEmit();
}

void Slot::_doEmit() {
    if (target) {
        if (cb) {
            cb(*this);
        } else if (!redirect.empty()) {
            auto so = dynamic_cast<SObject *>(target.ptr());
            if (so != nullptr) {
                so->signals().emit(redirect, data);
            }
        }
    } else if (cb) {
        cb(*this);
    }

    data = nullptr;
    tunnel = nullptr;

    ++emitedCount;
}

// --------------------------------------- slots

Slots::~Slots() {
    clear();
}

void Slots::clear() {
    __slots.clear();
}

void Slots::block() {
    ++__block;
}

void Slots::unblock() {
    --__block;
}

bool Slots::isblocked() const {
    return __block != 0;
}

void Slots::add(Slots::slot_type s) {
    __slots.emplace_back(s);
}

::std::set<SObject *> Slots::emit(Slot::data_type d, Slot::tunnel_type t) {
    ::std::set<SObject *> r;
    if (isblocked())
        return r;

    ::std::vector<decltype(__slots)::iterator> outs;
    auto slots = __slots;
    for (size_t i = 0, l = slots.size(); i < l; ++i) {
        auto s = slots.at(i);
        if (s->count && s->emitedCount >= s->count)
            continue; // 激活数控制

        // 激发信号
        s->signal = signal;
        s->sender = owner;
        s->emit(d, t);

        // 控制激活数
        if (s->count && s->emitedCount >= s->count) {
            outs.emplace_back(__slots.begin() + i);
        }

        // 阻断
        if (s->getVeto())
            break;
    }

    // 删除用完的slot
    for (auto const &iter:outs) {
        auto s = *iter;
        if (s->target)
            r.insert(s->target);
        __slots.erase(iter);
    }

    return r;
}

bool Slots::disconnect(Slot::callback_type cb) {
    bool r = false;
    for (auto iter = __slots.begin(); iter != __slots.end();) {
        auto s = *iter;

        if (s->_cb == cb) {
            r = true;
            iter = __slots.erase(iter);
        } else {
            ++iter;
        }
    }
    return r;
}

bool Slots::disconnect(Slot::callback_mem_type cb, SObject *target) {
    bool r = false;
    for (auto iter = __slots.begin(); iter != __slots.end();) {
        auto s = *iter;

        if (cb && s->_cbmem != cb) {
            ++iter;
            continue;
        }

        if (s->target == target) {
            r = true;
            iter = __slots.erase(iter);
        } else {
            ++iter;
        }
    }
    return r;
}

Slots::slot_type Slots::find_connected_function(Slot::callback_type cb) {
    auto fnd = ::std::find_if(__slots.begin(), __slots.end(), [=](__slots_type::value_type &s) {
        return s->_cb == cb;
    });
    return fnd == __slots.end() ? nullptr : *fnd;
}

Slots::slot_type Slots::find_connected_function(Slot::callback_mem_type cb, SObject *target) {
    auto fnd = ::std::find_if(__slots.begin(), __slots.end(), [=](__slots_type::value_type &s) {
        return s->_cbmem == cb && s->target == target;
    });
    return fnd == __slots.end() ? nullptr : *fnd;
}

Slots::slot_type Slots::find_redirected(signal_t const &sig, SObject *target) {
    auto fnd = ::std::find_if(__slots.begin(), __slots.end(), [=](__slots_type::value_type &s) {
        return s->redirect == sig && s->target == target;
    });
    return fnd == __slots.end() ? nullptr : *fnd;
}

bool Slots::is_connected(SObject *target) const {
    auto fnd = ::std::find_if(__slots.cbegin(), __slots.cend(), [=](__slots_type::value_type const &s) {
        return s->target == target;
    });
    return fnd != __slots.end();
}

// ---------------------------------------- signals

Signals::~Signals() {
    // 反向断开连接
    for (auto iter: __invs) {
        iter->owner.signals().disconnectOfTarget(&owner, false);
    }
    __invs.clear();
}

void Signals::clear() {
    // 清空反向的连接
    for (auto &iter:__invs) {
        iter->owner.signals().disconnectOfTarget(&owner, false);
    }

    // 清空slot的连接
    for (auto const &iter: __slots) {
        iter.second->clear();
    }
}

bool Signals::registerr(signal_t const &sig) {
    if (sig.empty()) {
        SS_LOG_WARN("不能注册一个空信号")
        return false;
    }

    if (__slots.find(sig) != __slots.end())
        return false;

    auto ss = ::std::make_shared<Slots>();
    ss->signal = sig;
    ss->owner = &owner;
    __slots.insert(::std::pair(sig, ss));
    return true;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::callback_type cb) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->find_connected_function(cb);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_cb = cb;
    s->cb = cb;
    ss->add(s);

    return s;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::callback_mem_type cb, SObject *target) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->find_connected_function(cb, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_cbmem = cb;
    s->cb = ::std::bind(cb, target, ::std::placeholders::_1);
    s->target = target;
    ss->add(s);

    __inv_connect(target);

    return s;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::callback_comm_type cb, SObject *target, Slot::callback_mem_type cbmem) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->find_connected_function(cbmem, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_cbmem = cbmem;
    s->cb = ::std::move(cb);
    s->target = target;
    ss->add(s);

    __inv_connect(target);

    return s;
}

bool Signals::isConnected(signal_t const &sig) const {
    auto fnd = __slots.find(sig);
    return fnd != __slots.end() && !fnd->second->__slots.empty();
}

Slots::slot_type Signals::redirect(signal_t const &sig, SObject *target) {
    return redirect(sig, sig, target);
}

Slots::slot_type Signals::redirect(signal_t const &sig1, signal_t const &sig2, SObject *target) {
    auto fnd = __slots.find(sig1);
    if (fnd == __slots.end())
        return nullptr;
    auto &ss = fnd->second;
    auto s = ss->find_redirected(sig2, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->redirect = sig2;
    s->target = target;
    ss->add(s);

    __inv_connect(target);

    return s;
}

void Signals::emit(signal_t const &sig, Slot::data_type d, Slot::tunnel_type t) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return;
    }

    auto &ss = fnd->second;
    auto targets = ss->emit(d, t);
    if (!targets.empty()) {
        // 收集所有被移除的target，并断开反向连接
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter)) {
                __inv_disconnect(iter);
            }
        }
    }
}

void Signals::disconnectOfTarget(SObject *target, bool inv) {
    if (target == nullptr)
        return;

    for (auto &iter : __slots) {
        iter.second->disconnect(nullptr, target);
    }

    if (inv)
        __inv_disconnect(target);
}

void Signals::disconnect(signal_t const &sig) {
    disconnect(sig, nullptr);
}

void Signals::disconnect(signal_t const &sig, Slot::callback_type cb) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end())
        return;
    auto &ss = fnd->second;

    if (cb == nullptr) {
        // 清除sig的所有插槽，自动断开反向引用
        ::std::set<SObject *> targets;
        for (const auto &iter:ss->__slots) {
            if (iter->target)
                targets.insert(iter->target);
        }
        ss->__slots.clear();
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter))
                __inv_disconnect(iter);
        }
    } else {
        ss->disconnect(cb);
    }
}

void Signals::disconnect(signal_t const &sig, Slot::callback_mem_type cb, SObject *target) {
    auto fnd = __slots.find(sig);
    if (fnd == __slots.end())
        return;
    auto &ss = fnd->second;

    if (cb == nullptr && target == nullptr) {
        // 清除sig的所有插槽，自动断开反向引用
        ::std::set<SObject *> targets;
        for (const auto &iter:ss->__slots) {
            if (iter->target)
                targets.insert(iter->target);
        }
        ss->__slots.clear();
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter))
                __inv_disconnect(iter);
        }
    } else {
        // 先清除对应的slot，再判断是否存在和target相连的插槽，如过不存在，则断开反向连接
        if (ss->disconnect(cb, target) && target && !isConnectedOfTarget(target)) {
            __inv_disconnect(target);
        }
    }
}

bool Signals::isConnectedOfTarget(SObject *target) const {
    auto fnd = ::std::find_if(__slots.cbegin(), __slots.cend(), [=](__slots_type::value_type const &ss) -> bool {
        return ss.second->is_connected(target);
    });
    return fnd != __slots.end();
}

void Signals::block(signal_t const &sig) {
    auto fnd = __slots.find(sig);
    if (fnd != __slots.end())
        fnd->second->block();
}

void Signals::unblock(signal_t const &sig) {
    auto fnd = __slots.find(sig);
    if (fnd != __slots.end())
        fnd->second->unblock();
}

bool Signals::isblocked(signal_t const &sig) const {
    auto fnd = __slots.find(sig);
    return fnd != __slots.end() ? fnd->second->isblocked() : false;
}

void Signals::__inv_connect(SObject *target) {
    if (target == nullptr)
        return;
    if (&target->signals() == this)
        return;
    target->signals().__invs.insert(this);
}

void Signals::__inv_disconnect(SObject *target) {
    if (target == nullptr)
        return;
    if (&target->signals() == this)
        return;
    target->signals().__invs.erase(this);
}

SS_END