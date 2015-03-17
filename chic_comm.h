/*
 * Chic communication library for inter-(hypo)model 
 * message exchange
 * Version 0.1
 * 
 * Copyright (c) 2015 Stelios Sfakianakis (FORTH-ICS)
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 */

#ifndef CHIC_COMM_H
#define CHIC_COMM_H

#include <string>
#include <exception>
#include <memory>

#include "buffer.hpp"

namespace chic {

	struct CommImpl;

    class Message {
        private:
			int id_;
            buffer payload_;
        public:
			Message(): id_(0) {}
			Message(int id): id_(id) {}

			int id() const { return id_; }
            buffer& payload() { return payload_; }
			bool isValid() const { return payload_.size() > 0; }

			//---------
			void setId(int id) { this->id_ = id; }
    };
    
    class Channel {
		friend struct CommImpl;
        public:
            std::string name() const;
            bool isInput() const;
            bool isOutput() const { return !isInput(); }

            class InChannel toInput();
            class OutChannel toOutput();

        protected:
            struct ChannelImpl* impl_;
			Channel() {}
            Channel(ChannelImpl* impl): impl_(impl) {}
    };

    class InChannel: public Channel {
        public:
            Message take();
            Message try_take(int timeout_millis);
    };
    class OutChannel: public Channel {
        public:
            void put(const char* payload, int nbytes);
			void put(const std::string& s) { put(s.c_str(), s.size()); }
			void put(const buffer& b) { put(b.data(), b.size()); }
    };

    struct not_found: public std::exception
    {
		not_found(const std::string& ch): w("Channel ")
		{
			w += ch;
			w += " was not found!";
		}
			
        virtual const char* what() const throw() {
            return w.c_str();
        }

		private:
		std::string w;
    };
    class Comm {
        public:
            Comm();
			~Comm();
            int init(int& argc, const char* argv[]);

			// ------ Registration of channels
            void register_input_channel(const std::string& name,
					const std::string& global_name);
            void register_output_channel(const std::string& name,
					const std::string& global_name);

			// ------ 
            InChannel get_input_channel(const std::string& name) const;
            OutChannel get_output_channel(const std::string& name) const;
        private:
            std::unique_ptr<CommImpl> impl_;
    };

}


#endif
