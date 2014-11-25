#include "logging.hpp"
#include "bot_server.hpp"
#include "serialization.hpp"

#include "bot_ca.hpp"
#include "bot_socket.hpp"
#include "user.pb.h"

namespace bot_avim {

	bot_server::bot_server(boost::shared_ptr<RSA> rsa, boost::shared_ptr<X509> x509_cert)
	: m_ca(rsa, x509_cert)
	{
		status = BOT_STATUS_OFFLINE;
		m_role = BOT_ROLE_INVALID;
		m_ios.reset(new boost::asio::io_service);
		LOG_DBG << "bot server constructor";
	}

	bot_server::~bot_server()
	{}
	
	bool bot_server::set_conn(std::string bot_addr,int bot_port,std::string server_addr,int server_port)
	{
		m_bot_addr = bot_addr;
		m_bot_port = bot_port;
		m_server_addr = server_addr;
		m_server_por = server_port;
		
		LOG_DBG << "bot addr:" << bot_addr;
		LOG_DBG << "bot port:" << bot_port;
		LOG_DBG << "server addr:" << server_addr;
		LOG_DBG << "server port:" << server_port;
		return true;
	}
	
	bool bot_server::set_role(bot_role role)
	{
		m_role = role;
		LOG_DBG << "ROLE:" <<role;
		return true;
	}
	
	bool bot_server::server_login_start()
	{
		proto::client_hello client_hello;
		client_hello.set_client("group");
		client_hello.set_version(0001);
		
		//dh ops
		m_ca.dh_generate_clientkey();
		client_hello.set_random_g(m_ca.get_random_g());
		client_hello.set_random_p(m_ca.get_random_p());
		client_hello.set_random_pub_key(m_ca.get_pubkey());
		
		std::string response = encode(client_hello);
		write_packet(response);
		return true;
		//av_router::encode(client_hello);
	}
	
	bool bot_server::handle_server_hello(google::protobuf::Message* msg)
	{
		proto::server_hello *server_hello = dynamic_cast<proto::server_hello*>(msg);

		//dh ops
		m_remote_addr.reset(new proto::av_address(av_address_from_string(server_hello->server_av_address())));
		std::string server_pubkey(server_hello->random_pub_key());
		m_ca.set_server_pubkey(server_pubkey);
		m_ca.dh_generate_client_shared_key();
		
		// send login msg
		std::string random_response = m_ca.private_encrypt(server_hello->random_pub_key());

		proto::login login_packet;
		login_packet.set_user_cert(i2d_X509(m_ca.get_shared_x509().get()));
		login_packet.set_encryped_radom_key(random_response);

		std::string response = encode(login_packet);
		write_packet(response);
		return true;
	}
	
	bool bot_server::handle_login_result(google::protobuf::Message* msg)
	{
		return true;
	}
	
	void bot_server::start()
	{
		m_socket.reset(new bot_socket(boost::ref(*m_ios), boost::ref(*this)));
		//m_socket = boost::make_shared<bot_socket>(boost::ref(*m_ios), boost::ref(*this));
		m_socket.get()->set_bot_addr(m_bot_addr, m_bot_port);
		m_socket.get()->set_server_addr(m_server_addr, m_server_por);
		m_socket.get()->start();
		server_login_start();
		m_ios->run();
	}

	void bot_server::stop()
	{
		m_ios->stop();
	}
	
	bool bot_server::write_packet(std::string &msg)
	{
		m_socket.get()->write_msg(msg);
	}
	
	bool bot_server::do_message(google::protobuf::Message* msg, bot_socket_ptr)
	{
		boost::shared_lock<boost::shared_mutex> l(m_server_mutex);
		const std::string name = msg->GetTypeName();
		
		if(name == "proto.server_hello")
			handle_server_hello(msg);
		return true;
	}
	
	bool bot_server::add_bot(const std::string& name, bot_role type)
	{
		return true;
	}

	bool bot_server::del_bot(const std::string& name)
	{
		return true;
	}

	void bot_server::continue_timer()
	{
	}
	
	void bot_server::dump_status()
	{
	}
	
}
