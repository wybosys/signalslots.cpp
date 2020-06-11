#include "../src/signals.hpp"

USE_SS;
using namespace std;

class A : public Object {
public:

    void test() {
        cout << "A" << endl;
    }
};

class B : public Object {
public:

    void proc(Slot &) {
        cout << "signal b: member function" << endl;
    }
};

class C : public Object {
public:

    void proc(Slot &) {
        cout << "signal c: member function" << endl;
    }
};

void test0() {
    Object a;

    a.signals().registerr("a");
    a.signals().connect("a", [&](Slot &s) {
        cout << "signal a: lambda" << endl;
        });

    B b;
    b.signals().registerr("b");
    b.signals().connect("b", [&](Slot &s) {
        cout << "signal b: lambda" << endl;
        });
    a.signals().once("a", &B::proc, &b);

    {
        C c;
        c.signals().registerr("c");
        a.signals().once("a", &C::proc, &c);
        a.signals().once("lost", &C::proc, &c);
    }

    auto f = [&](Slot &) {
        cout << "signal a: function" << endl;
    };
    a.signals().connect("a", f);

    a.signals().emit("a");
    //a.signals().disconnect("a", f);
    //a.signals().emit("a");
}

void test1()
{
    // 测试发出信号后删除自身
    auto tmp = new A();
    tmp->signals().registerr("a");
    tmp->signals().connect("a", [&](Slot &s) {
        delete tmp;
        });
    tmp->signals().connect("a", [&](Slot &s) {
        // 不应被调用，因为在上面已经析构
        tmp->test();
        });
    tmp->signals().emit("a");
}

int main() {
    // test0();
    test1();
    return 0;
}