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
    tmp->signals().registerr("b");
    tmp->signals().connect("a", [&](Slot &s) {
        // delete tmp;
        tmp->signals().connect("b", [&](Slot &s) {
            delete tmp;
            });
        });    
    tmp->signals().connect("a", [&](Slot &s) {
        tmp->signals().emit("b");
        });
    tmp->signals().emit("a");
}

void test2()
{
    A a;
    a.signals().registerr("a");
    a.signals().once("a", [&](Slot&s) {});
    a.signals().once("a", [&](Slot&s) {});
    a.signals().once("a", [&](Slot&s) {});
    a.signals().once("a", [&](Slot&s) {});
    a.signals().once("a", [&](Slot&s) {});
    a.signals().once("a", [&](Slot&s) {});
    a.signals().emit("a");
    if (a.signals().find("a")->size() != 0) {
        cerr << "按照次数进行连接的模块存在bug" << endl;
    }
}

void test3()
{
    // 测试对象之间的反向断开，不应输出任何东西
    A a;
    a.signals().registerr("a");
    {
        B b;
        a.signals().connect("a", &B::proc, &b);
    }
    a.signals().emit("a");
}

int main() {
    test0();
    test1();
    test2();
    test3();
    return 0;
}