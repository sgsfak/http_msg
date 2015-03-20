#include "chic_comm.h"
#include <ctime>
#include <iostream>
#include <exception>
#include <thread>
#include <atomic>
#include "jsonxx.h"


using namespace chic;
using namespace std;

struct Msg {
    enum Type { MSG, QUIT, JOIN} type_;
    string sender;
    string msg;

    Msg(Type t, const string& s): type_(t), sender(s) {}
    Msg(const string& s, const string& m): type_(MSG), sender(s), msg(m)
    {}

    string to_json() const;
    static Msg from_json(const string& s);
};
string Msg::to_json() const
{
    jsonxx::Object o;
    jsonxx::Value s, m, t;
    s.import(this->sender);
    m.import(this->msg);
    t.import( (int) this->type_ );
    o.import("type", t);
    o.import("sender", s);
    o.import("msg", m);
    return o.json();
}
Msg Msg::from_json(const string& s) 
{
    jsonxx::Object o;
    o.parse(s);
    string u = o.get<string>("sender", "");
    string g = o.get<string>("msg", "");
    int t = o.has<jsonxx::Number>("type") ? (int) o.get<jsonxx::Number>("type") : Msg::MSG;
    Msg m(u, g);
    m.type_ = (Type) t;
    return m;

}

void print_incoming(const char* me, const char* room, std::atomic<bool>& quited)
{
    Comm com;
    com.register_input_channel("in", room);

    InChannel in = com.get_input_channel("in");
    while (true) {
        Message msg;
        bool got = in.try_get(1000, msg);
        if (!got) {

            if (quited.load())
                break;
            else
                continue;
        }
        string s(msg.payload().data(), msg.payload().size());
        Msg m = Msg::from_json(s);
        if (m.sender != me)  {
            if (m.type_ == Msg::MSG)
                cout << "[" << msg.id() << "] " << m.sender << " says: " << m.msg << endl;
            else if (m.type_ == Msg::JOIN)
                cout << "[" << msg.id() << "] " << m.sender << " just joined room" << endl;
            else 
                cout << "[" << msg.id() << "] " << m.sender << " just left room" << endl;
        }

    }
}

int main(int argc, const char* argv[]) 
{
    Comm::init();

    if (argc < 3) {
        cout << "Usage: chat <user> <room>" << endl;
        return -1;
    }
    const char* sender = argv[1];
    const char* room = argv[2];

    std::atomic<bool> quited;
    std::atomic_init(&quited, false);
    thread t(print_incoming, sender, room, std::ref(quited));

    Comm com;
    com.register_output_channel("out", room);
    OutChannel out = com.get_output_channel("out");
    out.put(Msg(Msg::JOIN, sender).to_json());
    cout << "Welcome to online chat.. You are user: " <<
        sender << endl
        << ". You can say 'bye' to logout.." << endl
        << "> ";
    string line;
    while (getline(cin, line)) {
        if (line != "") {
            Msg m(sender, line);
            if (line == "bye") {
                out.put(Msg(Msg::QUIT, sender).to_json());
                std::atomic_store(&quited, true);
                break;
            }
            else
                out.put(m.to_json());
        }
        cout << endl << "> ";
    }
    t.join();

    return 0;

}

