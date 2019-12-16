#include "signals.h"

USE_SS;
using namespace std;

class B : public SObject {
public:

    void proc(Slot &) {
        cout << "signal b: member function" << endl;
    }
};

class C : public SObject {
public:

    void proc(Slot &) {
        cout << "signal c: member function" << endl;
    }
};

int main() {

    SObject a;

    a.signals().registerr("a");
    a.signals().connect("a", [](Slot &s) {
        cout << "signal a: lambda" << endl;
    });

    B b;
    b.signals().registerr("b");
    b.signals().connect("b", [](Slot &s) {
        cout << "signal b: lambda" << endl;
    });
    a.signals().redirect("a", "b", &b);
    a.signals().once("a", &B::proc, &b);

    {
        C c;
        c.signals().registerr("c");
        a.signals().once("a", &C::proc, &c);
        a.signals().once("lost", &C::proc, &c);
    }

    auto f = [](Slot &) {
        cout << "signal a: function" << endl;
    };
    a.signals().connect("a", f);

    a.signals().emit("a");
    a.signals().disconnect("a", f);
    a.signals().emit("a");

    return 0;
}