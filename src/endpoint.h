#ifndef _ENDPOINT_H
#define _ENDPOINT_H
#include <string>
#include <ctime>
#include "error.h"
#include "connection.h"
#include "blockwise.h"

namespace coap
{

class Endpoint {

public:
	Endpoint (const char *name)
	  : m_name{name}
	{}

	virtual ~Endpoint() = default;

	Endpoint(const Endpoint &) = delete;
	Endpoint & operator=(const Endpoint &) = delete;

public:
	virtual void registration_step(std::error_code &ec) = 0;
	virtual void transaction_step(std::error_code &ec) = 0;

	const std::string &name() const
	{ return m_name; }

protected:
	std::string 	m_name;
};

class ClientEndpoint : public Endpoint {

public:
	enum State {
		IDLE = 0,
		MAKE_REQUEST,
		SEND_REQUEST,
		RECEIVE_ANSWER,
		HANDLE_ANSWER,
		ERROR,
		COMPLETE,
		STATE_QTY
	};

private:
	void idle();
	void make_request();	
	void send_request();
	void receive_answer();
	void handle_answer();
	void error();
	void complete();

public:
	ClientEndpoint (const char *name, const ClientConnection &connection)
	  : Endpoint(name),
	  m_connection{connection},
	  m_packet{},
	  m_uri{},
	  m_attempts{0},
	  m_timeout{0},
	  m_mid{0},
	  m_received{false},
	  m_block2{},
	  m_currentState{IDLE},
	  m_nextState{IDLE},
	  m_ec{}
	{}

	~ClientEndpoint() = default;

public:
	void registration_step(std::error_code &ec) override;
	void transaction_step(std::error_code &ec) override;

	const ClientConnection &connection() const
	{ return m_connection; }

	const Packet & packet() const
	{ return static_cast<const Packet &>(m_packet); }

	const Uri    & uri() const
	{ return static_cast<const Uri &>(m_uri); }

	size_t attempts() const
	{ return m_attempts; }

	time_t timeout() const
	{ return m_timeout; }

	std::uint16_t mid() const
	{ return m_mid; }

	bool received() const
	{ return m_received; }

	const Block2 & block2() const
	{ return static_cast<const Block2 &>(m_block2); }

	State currentState() const
	{ return m_currentState; }

	State nextState() const
	{ return m_nextState; }

	bool isState(State state) const
	{ return (state >= 	IDLE && state <= COMPLETE); }

	void start()
	{ m_nextState = MAKE_REQUEST; }

	void stop()
	{ m_nextState = ERROR; }


private:
	const ClientConnection &m_connection;// reference to the external connection
	Packet 			 m_packet;			// CoAP packet instance
	Uri 			 m_uri; 			// destination URI
	size_t  		 m_attempts;		// quantity of the communication attempts	
	time_t           m_timeout; 		// receive timeout in seconds
	std::uint16_t    m_mid;    			// CoAP message identifier
	bool  			 m_received; 		// received packet flag
	Block2  		 m_block2; 			// Block2 instance
	State 			 m_currentState; 	// current state of Finite State Automate(FSA)
	State   		 m_nextState;       // next state of FSA
	std::error_code  m_ec; 				// error code
};

}// namespace coap
#endif
