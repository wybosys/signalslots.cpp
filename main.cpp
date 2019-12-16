#include "signals.h"

USE_SS;
using namespace std;

class B : public SObject {
public:

    void proc(Slot &) {
        cout << "signal b: member function" << endl;
    }
};

int main() {

    SObject a;
    B b;

    a.signals().registerr("a");
    b.signals().registerr("b");

    a.signals().connect("a", [](Slot &s) {
        cout << "signal a: lambda" << endl;
    });

    auto f = [](Slot &) {
        cout << "signal a: function" << endl;
    };
    a.signals().connect("a", f);
    a.signals().once("a", &B::proc, &b);

    a.signals().emit("a");
    a.signals().disconnect("a", f);
    a.signals().emit("a");

    return 0;
}