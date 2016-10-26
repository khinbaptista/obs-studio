/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <QtWidgets/QAction>
#include <QtWidgets/QWidget>
#include <QIcon>
#include <QMenu>
#include <QMessageBox>
#include <QDesktopWidget>
#include <QJsonDocument>
#include <QJsonObject>

#include "websocket-control.hpp"
#include "window-basic-main.hpp"

#include <iostream>
#include <stdexcept>

using namespace std;

WebsocketControl::WebsocketControl(obs_frontend_callbacks *api, bool start_listening){
	if (api == nullptr){
		std::cout << "INVALID API" << std::endl;
		throw new std::invalid_argument("obs_frontend_callbacks *api == nullptr");
	}

	this->api = api;
	this->main_window = static_cast<OBSBasic*>(api->obs_frontend_get_main_window());

	if (main_window == nullptr){
		std::cout << "NO MAIN WINDOW" << std::endl;
		throw new std::exception("Couldn't obtain main window from api");
	}

	//cria websocket que recebe comandos do mconf
	wsServer = new QWebSocketServer(QStringLiteral(""),
		QWebSocketServer::NonSecureMode, this);

	connect(main_window, SIGNAL(signal_StreamStarted()), this, SLOT(onStreamStarted()));
	connect(main_window, SIGNAL(signal_StreamStopped()), this, SLOT(onStreamStopped()));

	if (start_listening){
		Open();
	}
}

void WebsocketControl::Open() {
	if (wsServer->listen(QHostAddress("127.0.0.1"), WEBSOCKET_PORT)) {
		connect(wsServer, SIGNAL(newConnection()), this, SLOT(onClientConnected()));
		std::cout << "OBS: Server listening" << std::endl;
	}
	else {
		std::cerr << "Failed to start websocket server. Maybe the port was already taken?" << std::endl;
		throw new std::exception("Failed to start websocket server");
	}
}

void WebsocketControl::onClientConnected(){
	if (!wsClient.isNull() && wsClient->state() == QAbstractSocket::ConnectedState)
		return;

	wsClient = wsServer->nextPendingConnection();

	cout << "Client connected" << endl;

	connect(wsClient, SIGNAL(textMessageReceived(QString)), this, SLOT(onMessageReceived(QString)));
	connect(wsClient, SIGNAL(disconnected()), this, SLOT(onClientDisconnected()));
}

void WebsocketControl::onMessageReceived(QString str){
	WebsocketMessage m(str);

	std::cout << "WebsocketMessage: " << str.toStdString() << std::endl;

	if (!m.isValid) {
		std::cout << "WebsocketMessage is invalid: " <<
			std::endl << str.toStdString() << std::endl;
		return;
	}

	if (m.Type == MSG_TYPE_TRAYCONFIG)
		onTrayConfig(m.DisplayID, m.CaptureMouse);

	else if (m.Type == MSG_TYPE_STARTSTREAMING)
		StartStreaming(m);

	else if (m.Type == MSG_TYPE_STOPSTREAMING)
		StopStreaming();

	else if (m.Type == MSG_TYPE_CLOSE){
		Close();
	}

	else if (m.Type == MSG_TYPE_TOGGLE){
		ToggleVisibility();
	}
}

void WebsocketControl::onClientDisconnected(){
	cout << "Client disconnected" << endl;

	disconnect(wsClient, SIGNAL(textMessageReceived(QString)), 0, 0);
	disconnect(wsClient, SIGNAL(disconnected()), 0, 0);
	wsClient.clear();
}

void WebsocketControl::ToggleVisibility(){
	main_window->ToggleVisibility();
}

void WebsocketControl::onTrayConfig(int displayid, bool captureMouse){
	main_window->onSignal_TrayConfig(displayid, captureMouse);
}

void WebsocketControl::StartStreaming(WebsocketMessage c){
	if (!c.isValid) return;

	/* sample
{"type": "StartStreaming", "streamPath": "path", "streamName": "name", "displayId": 0,
"bitrate": 1000, "fps": 15, "width":800, "height": 600, "messageid":"4"}
	*/
	QDesktopWidget desktop;

	int width  = desktop.screenGeometry(c.DisplayID).width();
	int height = desktop.screenGeometry(c.DisplayID).height();

	cout << "Debug: Start streaming request" << endl;

	if (!main_window){
		std::cout << "main_window is null" << std::endl;
		return;
	}

	main_window->onSignal_StartStreaming(
		c.StreamName, c.StreamPath, width, height,
		c.Width, c.Height, c.FPS, c.Bitrate
	);

	api->obs_frontend_streaming_start();

	cout << "Debug: After api->obs_frontend_streaming_start()" << endl;
}

void WebsocketControl::StopStreaming(){
	api->obs_frontend_streaming_stop();
}

void WebsocketControl::Close(){
	StopStreaming();

	main_window->close();
}

void WebsocketControl::onStreamStarted() {
	SendMessage("{ \"type\": \"StreamStarted\" }");
}

void WebsocketControl::onStreamStopped() {
	SendMessage("{ \"type\": \"StreamStopped\" }");
}

void WebsocketControl::SendMessage(QString msg) {
	if (!wsClient.isNull() && wsClient->isValid()) {
		wsClient->sendTextMessage(msg);
	}
}

WebsocketMessage::WebsocketMessage() {
	isValid		= true;
	RawData		= "";		MessageID	= 0;

	Type		= "";		StreamPath	= "";
	StreamName	= "";		DisplayID	= 0;
	Width		= 0;		Height		= 0;
	FPS		= 0;		Bitrate		= 0;
	CaptureMouse	= false;
}

WebsocketMessage::WebsocketMessage(QString str) : WebsocketMessage() {
	bool debug = false;
	RawData = str;

	QJsonDocument d = QJsonDocument::fromJson(str.toUtf8());
	QJsonObject o = d.object();

	if (!o.contains("type")){
		isValid = false;
		return;
	}

	Type = o["type"].toString();

	if (o.contains("messageid")){
		MessageID = std::stoi(o["messageid"].toString().toStdString());
	}

	if (o.contains("debug")){
		debug = o["debug"].toBool();
	}

	if (debug){
		QMessageBox::information(nullptr, "WebsocketMessage",
			str, QMessageBox::StandardButton::Ok);
	}

	if (Type == "StartStreaming"){
		try{
			StreamPath	=	o["streamPath"].toString();
			StreamName	=	o["streamName"].toString();

			DisplayID	=	o["displayId"].toInt();
			Width		=	o["width"].toInt();
			Height		=	o["height"].toInt();
			FPS		=	o["fps"].toInt();
			Bitrate		=	o["bitrate"].toInt();
		}
		catch (std::exception e){
			isValid = false;
		}

	}
	else if (Type == "TrayConfig"){
		if (o.contains("Display"))
			DisplayID = o["Display"].toInt();
		else
			isValid = false;

		if (o.contains("CaptureMouse"))
			CaptureMouse = o["CaptureMouse"].toBool();
		else
			isValid = false;
	}

	if (debug && !isValid){
		QMessageBox::information(nullptr, "WebsocketMessage",
			"Message was invalid", QMessageBox::StandardButton::Ok);
	}
}
