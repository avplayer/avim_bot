#include "logging.hpp"
#include "avproto_wrapper.hpp"
#include "bot_group.hpp"

#include "avproto.hpp"
#include "avproto/avjackif.hpp"
#include "avproto/message.hpp"
#include "bot_service.hpp"

#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/evp.h>

namespace bot_avim {

	avproto_wrapper::avproto_wrapper(boost::asio::io_service& io_service, std::shared_ptr<RSA> key, std::shared_ptr<X509> crt)
		: m_io_service(io_service)
		, m_key(key)
		, m_crt(crt)
		, m_avkernel(io_service)
	{
			LOG_DBG << "avproto constructor";
	}

	avproto_wrapper::~avproto_wrapper()
	{}

	bool avproto_wrapper::register_service(bot_service *service)
	{
		m_service.reset(service);
	}

	bool avproto_wrapper::start()
	{
		boost::asio::spawn(m_io_service, std::bind(&avproto_wrapper::connect_coroutine, this, std::placeholders::_1));
	}
	
	const std::string& avproto_wrapper::get_local_addr()
	{
		return m_local_addr;
	}

	void avproto_wrapper::connect_coroutine(boost::asio::yield_context yield_context)
	{
		m_avif.reset(new avjackif(m_io_service));
		boost::asio::spawn(m_io_service, std::bind(&avproto_wrapper::login_coroutine, this, std::placeholders::_1));
	}

	bool avproto_wrapper::login_coroutine(boost::asio::yield_context yield_context)
	{
		std::shared_ptr<RSA> m_rsa_key = m_key;
		std::shared_ptr<X509> m_x509_cert = m_crt;

		m_avif->set_pki(m_rsa_key, m_x509_cert);
		auto _debug_host = getenv("AVIM_HOST");
		bool ret = m_avif->async_connect(_debug_host?_debug_host:"avim.avplayer.org", "24950", yield_context);
		//bool ret = m_avif->async_connect("127.0.0.1", "24950", yield_context);
		std::cout << "connect ok" << std::endl;
		if (m_avif->async_handshake(yield_context))
		{
			std::cout << "login success " << std::endl;
		}

		m_avkernel.add_interface(m_avif);
		std::string me_addr = av_address_to_string(*m_avif->if_address());
		m_avkernel.add_route(".+@.+", me_addr, m_avif->get_ifname(), 100);

		m_local_addr = av_address_to_string(*m_avif->if_address());
		
		// start message_receiver
		boost::asio::spawn(m_io_service, std::bind(&avproto_wrapper::handle_message, this, std::placeholders::_1));

		m_service.get()->notify(0, 1, 0);
		
		//reconnect
		m_avif->signal_notify_remove.connect([this](){
			m_service.get()->notify(0, 0, 0);
		});
		
		return true;
	}

	bool avproto_wrapper::handle_message(boost::asio::yield_context yield_context)
	{
		for(;;)
		{
			std::string  target, data, from;
			m_avkernel.async_recvfrom(target, data, yield_context);
			if(is_control_message(data))
				m_service.get()->handle_message(0,target, decode_control_message(data, from));
			else
				m_service.get()->handle_message(target, data);
		}
		return true;
	}

	bool avproto_wrapper::write_packet(const std::string &target, const std::string &pkt)
	{
		m_avkernel.async_sendto(target, pkt, [](boost::system::error_code ec){
			if(ec)
				std::cout << "send failed, msg: " << ec.message() << std::endl;
			else
				std::cout << "send ok" << std::endl;
		});
	}

}
