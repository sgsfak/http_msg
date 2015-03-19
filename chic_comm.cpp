#include "chic_comm.h"
#include "curl/curl.h"
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
		CURL* curl;

    public:
		CommImpl():curl(0) { curl = curl_easy_init(); }
		~CommImpl() { curl_easy_cleanup(curl); }
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
		bool get(ChannelImpl* chan, const char* queue, int millis, Message&);


		static size_t read_headers(void *ptr, size_t size, size_t n, void *arg)
		{
			map<string, string>& m = *static_cast<map<string, string>*>(arg);
			size_t c = n * size;
			string s(reinterpret_cast<char*>(ptr), c);
			auto i = s.find_first_of(':');
			if (i != string::npos) {
				auto k = s.substr(0, i);
				auto v = s.substr(i+1, c-1);
				m.insert(make_pair(k,v));
			}

			return c;

		}
		static size_t read_body(void *ptr, size_t size, size_t n, void *arg)
		{
			buffer* b = static_cast<buffer*>(arg);
			b->append(ptr, size*n);
			return size*n;
		}

};
void CommImpl::put(ChannelImpl* chan,
		const char* queue, const char* data, int nbytes)
{
	char uri[1024];
	char* q_esc = curl_easy_escape(curl, queue, strlen(queue));
	char* c_esc = curl_easy_escape(curl, chan->global_name.c_str(), chan->global_name.size());
	sprintf(uri,
			"http://%s:%d/q/%s/messages?routingKey=%s",
			MQ_HOST, MQ_PORT, q_esc, c_esc);
	curl_free(q_esc);
	curl_free(c_esc);
	// printf("posting to %s\n", uri);
	buffer b;
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	struct curl_slist *list = NULL;
	list = curl_slist_append(list, "Expect:");
	list = curl_slist_append(list, "Content-type:application/octet-stream");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, nbytes);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CommImpl::read_body);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
	//curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CommImpl::read_headers);

	auto ok = curl_easy_perform(curl);
	curl_slist_free_all(list);
	if(ok != CURLE_OK)
		throw chic::CommException(curl_easy_strerror(ok));
	long http_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (http_code / 100 != 2)
		throw chic::CommException("No success HTTP code!");

	string s(b.data(), b.size());
	jsonxx::Object res;
	res.parse(s);
	printf("Message sent with id=%d\n", 
			static_cast<int>(res.get<jsonxx::Number>("id")));

}
bool CommImpl::get(ChannelImpl* chan, const char* queue, int millis, Message& msg)
{
	int fromId = chan->lastMessageIdRecv + 1;
	char uri[1024];
	char* q_esc = curl_easy_escape(curl, queue, strlen(queue));
	char* c_esc = curl_easy_escape(curl, chan->global_name.c_str(), chan->global_name.size());
	sprintf(uri,
			"http://%s:%d/q/%s/messages?single=true&fromId=%d&routingKey=%s&timeoutMs=%d",
			MQ_HOST, MQ_PORT,
			q_esc, fromId, c_esc, millis);
	curl_free(q_esc);
	curl_free(c_esc);
	// printf("Getting from %s\n", uri);
	buffer b;
	map<string, string> headers;
	curl_easy_setopt(curl, CURLOPT_URL, uri);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CommImpl::read_body);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CommImpl::read_headers);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);

	auto ok = curl_easy_perform(curl);
	if(ok != CURLE_OK)
		throw chic::CommException(curl_easy_strerror(ok));
	long http_code;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
	if (http_code / 100 != 2)
		throw chic::CommException("No success HTTP code!");

	auto idx =  headers.find("QDB-Id");
	if (idx == headers.end()) 
		return false;
	msg.setId( std::stoi( idx->second ) );
	msg.payload().swap(b);
	chan->lastMessageIdRecv = msg.id();
	// printf("got msg with id= %d\n", msg.id());

	return true;
}
int Comm::init(int& argc, const char* argv[])
{
	
	curl_global_init(CURL_GLOBAL_ALL);
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

bool InChannel::try_get(int millis, Message& msg)
{
   	bool ret = impl_->comm->get(impl_, "foo", millis, msg);
	return ret;
}
Message InChannel::get()
{
	Message msg;
   	impl_->comm->get(impl_, "foo", 0, msg);
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




