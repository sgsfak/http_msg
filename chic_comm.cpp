#include "chic_comm.h"
#include "happyhttp.h"
#include "jsonxx.h"
#include "buffer.hpp"
#include <map>
#include <memory>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <assert.h>


using namespace std;

struct chic::ChannelImpl {
	chic::CommImpl* comm;
    string local_name;
	string global_name;
	bool is_input;
	int lastMessageIdRecv;

	ChannelImpl(chic::CommImpl* c,
			const string& ln, const string& gn, bool is_in):
		comm(c), local_name(ln), global_name(gn), is_input(is_in), lastMessageIdRecv(0) {}
};

#define MQ_HOST "127.0.0.1"
#define MQ_PORT 9554

namespace chic_util {

	int netstring_to_str(const unsigned char* bytes, int len,
			string& out_s) {

		string s(reinterpret_cast<char const*>(bytes), len);
		int i = s.find(':'); // a netstring, preceded by length
		int sz = std::stoi(s.substr(0, i));
		string rest= s.substr(i+1, sz);
		out_s.swap(rest);
		return sz;
	}


}

using namespace chic;



string Channel::name() const 
{
    return this->impl_->local_name;
}

struct chic::CommImpl {
    private:
        map<string, shared_ptr<ChannelImpl> > channels;
		happyhttp::Connection conn;

    public:
		CommImpl():conn(MQ_HOST, MQ_PORT) {}
        void add_inchan(const string& name, const string& gname) {
            shared_ptr<ChannelImpl> p(new ChannelImpl(this, name, gname, true));
            this->channels.insert(make_pair(name, p));
        }

        void add_outchan(const string& name, const string& gname) {
            shared_ptr<ChannelImpl> p(new ChannelImpl(this, name, gname, false));
            this->channels.insert(make_pair(name, p));
        }

        Channel get_channel(const string& name) {
            auto it = channels.find(name);
            if (it == channels.end())
                throw not_found(name);
			Channel ch(it->second.get());
			return ch;
        }
		void put(ChannelImpl* chan, 
				const char* queue, const char* data, int nbytes);
		chic::Message take(ChannelImpl* chan, const char* queue, int millis=0);

};
void CommImpl::put(ChannelImpl* chan,
		const char* queue, const char* data, int nbytes)
{
	jsonxx::Object res;
	conn.setcallbacks( 0, 
			[](const happyhttp::Response* r, void* arg, const unsigned char* bytes, int len) {
			string s(reinterpret_cast<char const*>(bytes), len);
			jsonxx::Object* res = static_cast<jsonxx::Object*>(arg);
			res->parse(s);
			if (r->getstatus() / 100 == 2 && res->has<jsonxx::Number>("id")) 
			  printf("Message sent with id=%d\n", 
					  static_cast<int>(res->get<jsonxx::Number>("id")));
			},
		   	0, &res );

	char uri[1024];
	sprintf(uri, "/q/%s/messages?routingKey=%s",
			queue, chan->global_name.c_str());
	// printf("posting to %s\n", uri);
	conn.request("POST",uri, 0,
			reinterpret_cast<const unsigned char*>(data), nbytes);

	while( conn.outstanding() )
		conn.pump();


}
Message CommImpl::take(ChannelImpl* chan, const char* queue, int millis)
{
	Message msg;
	conn.setcallbacks( 
			[](const happyhttp::Response* r, void* arg) {
			Message* m = static_cast<Message*>(arg);
			printf("Got %d\n", r->getstatus());
			const char* h = r->getheader("QDB-Id");
			if (r->getstatus() / 100 == 2 && h) {
			  m->setId( std::stoi(h) );
			}
			},
			[](const happyhttp::Response* r, void* arg, const unsigned char* bytes, int len) {
			printf("**Got %d\n", len);
			Message* m = static_cast<Message*>(arg);
			m->payload().append(reinterpret_cast<char const*>(bytes), len);
			},
		   	[](const happyhttp::Response* r, void* arg) {
			  printf("Completed\n");
			  },
			&msg );

	int fromId = chan->lastMessageIdRecv + 1;
	char uri[1024];
	sprintf(uri, "/q/%s/messages?single=true&fromId=%d&routingKey=%s&timeoutMs=%d",
			queue, fromId, chan->global_name.c_str(), millis);
	printf("getting from %s\n", uri);
	conn.request( "GET", uri);

	while( conn.outstanding() )
		conn.pump();

	if (msg.isValid()) 
		chan->lastMessageIdRecv = msg.id();
	printf("got msg with id= %d\n", msg.id());

	return msg;
}
int Comm::init(int& argc, const char* argv[])
{
    return argc;
}

void Comm::register_input_channel(const string& name, const string& gname)
{
    this->impl_->add_inchan(name, gname);
}
void Comm::register_output_channel(const string& name, const string& gname)
{
    this->impl_->add_outchan(name, gname);
}

void OutChannel::put(const char* payload, int nbytes)
{
	assert(impl_);
	impl_->comm->put(impl_, "foo", payload, nbytes);
}

Message InChannel::try_take(int millis)
{
	Message msg = impl_->comm->take(impl_, "foo", millis);
	return msg;
}
Message InChannel::take()
{
	Message msg = impl_->comm->take(impl_, "foo", 0);
	return msg;
}
InChannel Comm::get_input_channel(const string& name) const
{
    auto ch = this->impl_->get_channel(name);
	if (!ch.isInput())
		throw not_found(name);
	return ch.toInput();
}
OutChannel Comm::get_output_channel(const string& name) const
{
	auto ch = this->impl_->get_channel(name);
	if (ch.isInput())
		throw not_found(name);
	return ch.toOutput();
}

Comm::Comm(): impl_(new CommImpl)
{
}

Comm::~Comm() = default;

bool Channel::isInput() const {
	assert(this->impl_);
	return this->impl_->is_input;
}

InChannel Channel::toInput() {
	assert(this->isInput());
	InChannel ch;
	ch.impl_ = this->impl_;
	return ch;
}
OutChannel Channel::toOutput() {
	assert(!this->isInput());
	OutChannel ch;
	ch.impl_ = this->impl_;
	return ch;
}




