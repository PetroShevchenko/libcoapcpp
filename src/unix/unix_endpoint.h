#include "endpoint.h"
#include "connection.h"
#include "blockwise.h"
#include "core_link.h"
#include "senml_json.h"

using namespace coap;

namespace Unix
{

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
	ClientEndpoint (const char *name, ClientConnection *connection)
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

	ClientConnection *connection()
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
	ClientConnection *m_connection;		// pointer to the external connection
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

class ServerEndpoint : public Endpoint {// Attention! ServerEndpoint class isn't completed

public:
	enum State {
		IDLE = 0,
		RECEIVE_REQUEST,
		ERROR,
		COMPLETE,
		STATE_QTY
	};

private:
	void idle();
	void receive_request();
	void error();
	void complete();

public:
	ServerEndpoint(const char *name, ServerConnection *connection)
	  : Endpoint(name),
	  m_connection{connection},
	  m_coreLink{},
	  m_senmlJson{},
	  m_receiving{false},
	  m_sending{false},
	  m_received{false},
	  m_timeout{0},
	  m_currentState{IDLE},
	  m_nextState{IDLE},
	  m_ec{}
	{}

	ServerEndpoint(const char *name,
		const char *coreLink,
		ServerConnection *connection,
		std::error_code &ec
		)
	  : Endpoint(name),
	  m_connection{connection},
	  m_coreLink{coreLink, ec},
	  m_senmlJson{},
	  m_receiving{false},
	  m_sending{false},
	  m_received{false},
	  m_timeout{0},
	  m_currentState{IDLE},
	  m_nextState{IDLE},
	  m_ec{}
	{}

	~ServerEndpoint() = default;

public:
	void registration_step(std::error_code &ec) override;
	void transaction_step(std::error_code &ec) override;

	ServerConnection *connection()
	{ return m_connection; }

	State currentState() const
	{ return m_currentState; }

	State nextState() const
	{ return m_nextState; }

	bool receiving() const
	{ return m_receiving; }

	bool sending() const
	{ return m_sending; }

	bool received() const
	{ return m_received; }

	time_t timeout() const
	{ return m_timeout; }

	void timeout(time_t t)
	{ m_timeout = t; }

	void received(bool value)
	{ m_received = value; }

	void start()
	{ m_nextState = RECEIVE_REQUEST; }

	void stop()
	{ m_nextState = COMPLETE; }

private:
	ServerConnection *m_connection;		// pointer to the external connection	
	CoreLink 		 m_coreLink;		// CoRE Link payload parser
	SenmlJson 		 m_senmlJson;		// SenML JSON payload parser
	bool 			 m_receiving;		// need to receive a packet
	bool  			 m_sending; 		// need to send a packet
	bool             m_received; 		// something received to the buffer
	time_t           m_timeout; 		// receive timeout in seconds
	State 			 m_currentState; 	// current state of Finite State Automate(FSA)
	State   		 m_nextState;       // next state of FSA
	std::error_code  m_ec; 				// error code
};

} //namespace Unix
