// project includes
#include <mqtt/mqtt.h>

#include <base/HyperHdrInstance.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QHostInfo>

QString HYPERHDRAPI = QStringLiteral("HyperHDR/JsonAPI");
QString HYPERHDRAPI_RESPONSE = QStringLiteral("HyperHDR/JsonAPI/response");

mqtt::mqtt(QObject* _parent)
	: QObject(_parent)
	, _jsonPort(8090)
	, _log(Logger::getInstance("MQTT"))
	, _jsonAPI(nullptr)
	, _clientInstance(nullptr)
{
}

mqtt::~mqtt()
{
	stop();
}

void mqtt::sendPayload(QJsonObject payload) {
	if (_clientInstance != nullptr) {
		QMQTT::Message report;
		report.setTopic(HYPERHDRAPI_RESPONSE);
		report.setQos(2);
		report.setPayload(QJsonDocument(payload).toJson());
		_clientInstance->publish(report);
		//Debug(_log, "Sending payload: '%s'", QJsonDocument(payload).toJson());
	}
}

void mqtt::handleCallback(QJsonObject obj)
{
	if (_clientInstance != nullptr) {
		sendPayload(obj);
	}
}

void mqtt::sendHelloOrByePayload(bool connected) {
	if (_clientInstance != nullptr) {
		QJsonObject payload;
		if (connected) {
			payload.insert("command", "alive");
		} else {
			payload.insert("command", "bye");
		}
		QJsonObject info;
		info.insert("on", connected);
		payload.insert("success", true);
		payload.insert("info", info);
		sendPayload(payload);
	}
}

void mqtt::start(QString host, int port, QString username, QString password, bool is_ssl, bool ignore_ssl_errors)
{

	if (_clientInstance != nullptr)
		return;

	Debug(_log, "Starting the MQTT connection. Address: %s:%i. Protocol: %s. Authentication: %s, Ignore errors: %s",
				QSTRING_CSTR(host), port, (is_ssl) ? "SSL": "NO SSL", (!username.isEmpty() || !password.isEmpty()) ? "YES" : "NO", (ignore_ssl_errors) ? "YES" : "NO");

	QHostAddress adr(host);


	QHostInfo info = QHostInfo::fromName(host);
	if (!info.addresses().isEmpty()) {
    	adr = info.addresses().first();
	}


	if (is_ssl)
	{
		QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
		_clientInstance = new QMQTT::Client(host, port, sslConfig);
	}
	else
		_clientInstance = new QMQTT::Client(adr, port);


	QString clientId = QString("HyperHDR:%1").arg(QHostInfo::localHostName());

	_clientInstance->setClientId(clientId);

	if (!username.isEmpty())
		_clientInstance->setUsername(username);

	if (!password.isEmpty())
		_clientInstance->setPassword(password.toLocal8Bit());

	if (is_ssl && ignore_ssl_errors)
	{
		QObject::connect(_clientInstance, &QMQTT::Client::sslErrors, [&](const QList<QSslError>& errors) {
			_clientInstance->ignoreSslErrors();
			});
	}
	QObject::connect(_clientInstance, &QMQTT::Client::error, this, &mqtt::error);
	QObject::connect(_clientInstance, &QMQTT::Client::connected, this, &mqtt::connected);
	QObject::connect(_clientInstance, &QMQTT::Client::received, this, &mqtt::received);
	_clientInstance->connectToHost();

	if (_jsonAPI == nullptr) {
		const QString client = QStringLiteral("MQTT");
		_jsonAPI = new JsonAPI(client, _log, true, this, false);
		connect(_jsonAPI, &JsonAPI::callbackMessage, this, &mqtt::handleCallback);
		_jsonAPI->initialize();
	}
}

void mqtt::stop()
{
	Debug(_log,"Stop");
	sendHelloOrByePayload(false);
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(150);
	loop.exec();

	if (_clientInstance != nullptr)
	{
		delete _clientInstance;
		_clientInstance = nullptr;
	}
	if (_jsonAPI != nullptr)
	{
		delete _jsonAPI;
		_jsonAPI = nullptr;
	}
}

void mqtt::connected()
{
	Debug(_log, "Connected");

	if (_clientInstance != nullptr)
	{
		_clientInstance->subscribe(HYPERHDRAPI, 2);
		sendHelloOrByePayload(true);
	}
}

void mqtt::error(const QMQTT::ClientError error)
{
	QString message;
	switch (error)
	{
		case(QMQTT::ClientError::UnknownError): message = "UnknownError"; break;
		case(QMQTT::ClientError::SocketConnectionRefusedError): message = "SocketConnectionRefusedError"; break;
		case(QMQTT::ClientError::SocketRemoteHostClosedError): message = "SocketRemoteHostClosedError"; break;
		case(QMQTT::ClientError::SocketHostNotFoundError): message = "SocketHostNotFoundError"; break;
		case(QMQTT::ClientError::SocketAccessError): message = "SocketAccessError"; break;
		case(QMQTT::ClientError::SocketResourceError): message = "SocketResourceError"; break;
		case(QMQTT::ClientError::SocketTimeoutError): message = "SocketTimeoutError"; break;
		case(QMQTT::ClientError::SocketDatagramTooLargeError): message = "SocketDatagramTooLargeError"; break;
		case(QMQTT::ClientError::SocketNetworkError): message = "SocketNetworkError"; break;
		case(QMQTT::ClientError::SocketAddressInUseError): message = "SocketAddressInUseError"; break;
		case(QMQTT::ClientError::SocketAddressNotAvailableError): message = "SocketAddressNotAvailableError"; break;
		case(QMQTT::ClientError::SocketUnsupportedSocketOperationError): message = "SocketUnsupportedSocketOperationError"; break;
		case(QMQTT::ClientError::SocketUnfinishedSocketOperationError): message = "SocketUnfinishedSocketOperationError"; break;
		case(QMQTT::ClientError::SocketProxyAuthenticationRequiredError): message = "SocketProxyAuthenticationRequiredError"; break;
		case(QMQTT::ClientError::SocketSslHandshakeFailedError): message = "SocketSslHandshakeFailedError"; break;
		case(QMQTT::ClientError::SocketProxyConnectionRefusedError): message = "SocketProxyConnectionRefusedError"; break;
		case(QMQTT::ClientError::SocketProxyConnectionClosedError): message = "SocketProxyConnectionClosedError"; break;
		case(QMQTT::ClientError::SocketProxyConnectionTimeoutError): message = "SocketProxyConnectionTimeoutError"; break;
		case(QMQTT::ClientError::SocketProxyNotFoundError): message = "SocketProxyNotFoundError"; break;
		case(QMQTT::ClientError::SocketProxyProtocolError): message = "SocketProxyProtocolError"; break;
		case(QMQTT::ClientError::SocketOperationError): message = "SocketOperationError"; break;
		case(QMQTT::ClientError::SocketSslInternalError): message = "SocketSslInternalError"; break;
		case(QMQTT::ClientError::SocketSslInvalidUserDataError): message = "SocketSslInvalidUserDataError"; break;
		case(QMQTT::ClientError::SocketTemporaryError): message = "SocketTemporaryError"; break;
		case(1 << 16): message = "MqttUnacceptableProtocolVersionError"; break;
		case(QMQTT::ClientError::MqttIdentifierRejectedError): message = "MqttIdentifierRejectedError"; break;
		case(QMQTT::ClientError::MqttServerUnavailableError): message = "MqttServerUnavailableError"; break;
		case(QMQTT::ClientError::MqttBadUserNameOrPasswordError): message = "MqttBadUserNameOrPasswordError"; break;
		case(QMQTT::ClientError::MqttNotAuthorizedError): message = "MqttNotAuthorizedError"; break;
		case(QMQTT::ClientError::MqttNoPingResponse): message = "MqttNoPingResponse"; break;
	}
	Error(_log, "Error: %s", QSTRING_CSTR(message));
}

void mqtt::handleSettingsUpdate(settings::type type, const QJsonDocument& config)
{
	if (type == settings::type::MQTT)
	{
		QJsonObject obj = config.object();


		bool enabled = obj["enable"].toBool();
		QString host = obj["host"].toString();
		int port = obj["port"].toInt();
		QString username = obj["username"].toString();
		QString password = obj["password"].toString();
		bool is_ssl = obj["is_ssl"].toBool();
		bool ignore_ssl_errors = obj["ignore_ssl_errors"].toBool();


		stop();
		if (enabled)
			start(host, port, username, password, is_ssl, ignore_ssl_errors);
	}
	else if (type == settings::type::WEBSERVER)
	{
		QJsonObject obj = config.object();
		_jsonPort = obj["port"].toInt();
	}
}

void mqtt::received(const QMQTT::Message& message)
{
	QString topic = message.topic();
	QString payload = QString().fromLocal8Bit(message.payload());

	if (QString::compare(HYPERHDRAPI, topic) == 0 && payload.length() > 0)
	{
		_jsonAPI->handleMessage(payload);
	}
}
