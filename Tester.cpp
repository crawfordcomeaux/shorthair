#include "shorthair/Shorthair.hpp"
#include "MersenneTwister.hpp"
using namespace cat;
using namespace shorthair;

#include <iostream>
#include <iomanip>
using namespace std;


//// ZeroLoss Classes

class ZeroLossServer;
class ZeroLossClient;

class ZeroLossServer : IShorthair {
	friend class ZeroLossClient;

	Shorthair _codec;
	ZeroLossClient *_client;
	MersenneTwister *_prng;
	u32 _next;

	// Called with the latest data packet from remote host
	virtual void OnPacket(void *packet, int bytes);

	// Called with the latest OOB packet from remote host
	virtual void OnOOB(const u8 *packet, int bytes);

	// Send raw data to remote host over UDP socket
	virtual void SendData(void *buffer, int bytes);

public:
	void Accept(ZeroLossClient *client, const u8 *key, MersenneTwister *prng);
	void Tick();
};


class ZeroLossClient : IShorthair {
	friend class ZeroLossServer;

	Shorthair _codec;
	ZeroLossServer *_server;
	MersenneTwister *_prng;

	// Called with the latest data packet from remote host
	virtual void OnPacket(void *packet, int bytes);

	// Called with the latest OOB packet from remote host
	virtual void OnOOB(const u8 *packet, int bytes);

	// Send raw data to remote host over UDP socket
	virtual void SendData(void *buffer, int bytes);

public:
	void Connect(ZeroLossServer *server, MersenneTwister *prng);
	void Tick();
};


//// ZeroLossServer

// Called with the latest data packet from remote host
void ZeroLossServer::OnPacket(void *packet, int bytes) {
	CAT_EXCEPTION();
}

// Called with the latest OOB packet from remote host
void ZeroLossServer::OnOOB(const u8 *packet, int bytes) {
	CAT_EXCEPTION();
}

// Send raw data to remote host over UDP socket
void ZeroLossServer::SendData(void *buffer, int bytes) {
	_client->_codec.Recv(buffer, bytes);
}

void ZeroLossServer::Accept(ZeroLossClient *client, const u8 *key, MersenneTwister *prng) {
	_client = client;
	_prng = prng;

	Settings settings;
	settings.initiator = false;
	settings.target_loss = 0.0001;
	settings.min_loss = 0.03;
	settings.min_delay = 100;
	settings.max_delay = 3000;
	settings.max_data_size = 1350;
	settings.interface = this;

	_codec.Initialize(key, settings);

	_next = 0;
}

void ZeroLossServer::Tick() {
	// Send data at a steady rate

	// >10 "MBPS" if packet payload is 1350 bytes
	for (int ii = 0; ii < 16*5; ++ii) {
		_codec.Send(&_next, 4);
		++_next;
	}

	_codec.Tick();
}


//// ZeroLossClient

// Called with the latest data packet from remote host
void ZeroLossClient::OnPacket(void *packet, int bytes) {
	CAT_ENFORCE(bytes == 4);

	u32 id = *(u32*)packet;

	cout << id << endl;
}

// Called with the latest OOB packet from remote host
void ZeroLossClient::OnOOB(const u8 *packet, int bytes) {
	CAT_EXCEPTION();
}

// Send raw data to remote host over UDP socket
void ZeroLossClient::SendData(void *buffer, int bytes) {
	// Simulate loss
	if (_prng->GenerateUnbiased(1, 100) <= 3) {
		cout << "LOSS" << endl;
	} else {
		_server->_codec.Recv(buffer, bytes);
	}
}

void ZeroLossClient::Connect(ZeroLossServer *server, MersenneTwister *prng) {
	_server = server;
	_prng = prng;

	Settings settings;
	settings.initiator = true;
	settings.target_loss = 0.0001;
	settings.min_loss = 0.03;
	settings.min_delay = 100;
	settings.max_delay = 3000;
	settings.max_data_size = 1350;
	settings.interface = this;

	u8 key[SKEY_BYTES] = {0};

	_codec.Initialize(key, settings);

	server->Accept(this, key, prng);
}

void ZeroLossClient::Tick() {
	_codec.Tick();
}


//// ZeroLossTest

void ZeroLossTest() {
	Clock clock;
	clock.OnInitialize();

	MersenneTwister prng;
	prng.Initialize(0);

	ZeroLossClient client;
	ZeroLossServer server;

	// Simulate connection start
	client.Connect(&server, &prng);

	for (;;) {
		// Wait 10 ms
		Clock::sleep(10);

		// Tick at ~10 ms rate
		client.Tick();
		server.Tick();
	}

	clock.OnFinalize();
}



int main()
{
	ZeroLossTest();

	return 0;
}

