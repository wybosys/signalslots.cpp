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
    if (target) {
        if (cb) {
            cb(*this);
        } else if (!redirect.empty()) {
            auto so = dynamic_cast<Object *>(target.ptr());
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

    for (auto iter = _slots.begin(); iter != _slots.end();)  {
        auto &s = *iter;
        if (s->count && (s->emitedCount >= s->count))
            continue; // 激活数控制

        // 激发信号
        s->signal = signal;
        s->sender = owner;
        s->emit(d, t);

        // 控制激活数
        if (s->count && s->emitedCount >= s->count) {
            if (s->target)
                r.insert(s->target);
            iter = _slots.erase(iter);
        }
        else {
            ++iter;
        }

        // 阻断
        if (s->getVeto())
            break;
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

Slots::slot_type Slots::find_connected_function(Slot::pfn_callback_type cb) {
    auto fnd = ::std::find_if(_slots.begin(), _slots.end(), [=](slots_type::value_type &s) {
        return s->_pfn_cb == cb;
    });
    return fnd == _slots.end() ? nullptr : *fnd;
}

Slots::slot_type Slots::find_connected_function(Slot::pfn_membercallback_type cb, Object *target) {
    auto fnd = ::std::find_if(_slots.begin(), _slots.end(), [=](slots_type::value_type &s) {
        return s->_pfn_memcb == cb && s->target == target;
    });
    return fnd == _slots.end() ? nullptr : *fnd;
}

Slots::slot_type Slots::find_redirected(signal_t const &sig, Object *target) {
    auto fnd = ::std::find_if(_slots.begin(), _slots.end(), [=](slots_type::value_type &s) {
        return s->redirect == sig && s->target == target;
    });
    return fnd == _slots.end() ? nullptr : *fnd;
}

bool Slots::is_connected(Object *target) const {
    auto fnd = ::std::find_if(_slots.cbegin(), _slots.cend(), [=](slots_type::value_type const &s) {
        return s->target == target;
    });
    return fnd != _slots.end();
}

// ---------------------------------------- signals

Signals::~Signals() {
    clear();
}

void Signals::clear() {
    // 清空反向的连接
    for (auto &iter:_invs) {
        iter->owner->signals().disconnectOfTarget(owner, false);
    }
    _invs.clear();

    // 清空slot的连接
    for (auto const &iter: _signals) {
        iter.second->clear();
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
    ss->signal = sig;
    ss->owner = owner;
    _signals.insert(::std::make_pair(sig, ss));
    return true;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::pfn_callback_type cb) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->find_connected_function(cb);
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
    auto s = ss->find_connected_function(cb, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_pfn_memcb = cb;
    s->cb = ::std::bind(cb, target, ::std::placeholders::_1);
    s->target = target;
    ss->add(s);

    _inv_connect(target);

    return s;
}

Slots::slot_type Signals::connect(signal_t const &sig, Slot::callback_type cb, Object *target, Slot::pfn_membercallback_type cbmem) {
    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return nullptr;
    }
    auto ss = fnd->second;

    // 判断是否已经连接
    auto s = ss->find_connected_function(cbmem, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->_pfn_memcb = cbmem;
    s->cb = ::std::move(cb);
    s->target = target;
    ss->add(s);

    _inv_connect(target);

    return s;
}

bool Signals::isConnected(signal_t const &sig) const {
    auto fnd = _signals.find(sig);
    return fnd != _signals.end() && !fnd->second->_slots.empty();
}

Slots::slot_type Signals::redirect(signal_t const &sig, Object *target) {
    return redirect(sig, sig, target);
}

Slots::slot_type Signals::redirect(signal_t const &sig1, signal_t const &sig2, Object *target) {
    auto fnd = _signals.find(sig1);
    if (fnd == _signals.end())
        return nullptr;
    auto &ss = fnd->second;
    auto s = ss->find_redirected(sig2, target);
    if (s)
        return s;

    s = ::std::make_shared<Slot>();
    s->redirect = sig2;
    s->target = target;
    ss->add(s);

    _inv_connect(target);

    return s;
}

void Signals::emit(signal_t const &sig, Slot::data_type d, Slot::tunnel_type t) {
    // 保护signals，避免运行期被释放
    ::std::shared_ptr<Signals> lifekeep(owner->_s);

    auto fnd = _signals.find(sig);
    if (fnd == _signals.end()) {
        SS_LOG_WARN("对象信号 " + sig + " 不存在")
        return;
    }

    auto &ss = fnd->second;
    auto targets = ss->emit(d, t);
    if (!targets.empty()) {
        // 收集所有被移除的target，并断开反向连接
        for (auto &iter:targets) {
            if (!isConnectedOfTarget(iter)) {
                _inv_disconnect(iter);
            }
        }
    }
}

void Signals::disconnectOfTarget(Object *target, bool inv) {
    if (target == nullptr)
        return;

    for (auto &iter : _signals) {
        iter.second->disconnect(nullptr, target);
    }

    if (inv)
        _inv_disconnect(target);
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
            if (!isConnectedOfTarget(iter))
                _inv_disconnect(iter);
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
            if (!isConnectedOfTarget(iter))
                _inv_disconnect(iter);
        }
    } else {
        // 先清除对应的slot，再判断是否存在和target相连的插槽，如过不存在，则断开反向连接
        if (ss->disconnect(cb, target) && target && !isConnectedOfTarget(target)) {
            _inv_disconnect(target);
        }
    }
}

bool Signals::isConnectedOfTarget(Object *target) const {
    for (auto iter = _signals.cbegin(); iter != _signals.end(); ++iter) {
        if (iter->second->is_connected(target)) {
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

void Signals::_inv_connect(Object *target) {
    if (target == nullptr)
        return;
    if (&target->signals() == this)
        return;
    target->signals()._invs.insert(this);
}

void Signals::_inv_disconnect(Object *target) {
    if (target == nullptr)
        return;
    if (&target->signals() == this)
        return;
    target->signals()._invs.erase(this);
}

SS_END