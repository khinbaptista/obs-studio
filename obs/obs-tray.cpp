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

#include "obs-tray.hpp"

// exibe corretamente strings acentuadas
#define ptbr QString::fromLatin1

OBSTray::OBSTray()
{
	//cria websocket que recebera comandos do mconf
	wbsServer = new QWebSocketServer(QStringLiteral(""),
		QWebSocketServer::NonSecureMode, this);
	if (wbsServer->listen(QHostAddress::Any, 2424)) {
		connect(wbsServer, SIGNAL(newConnection()), this, SLOT(AddClient()));
	}

	defaultIcon = QIcon(":/settings/images/settings/video-display-3.png");
	playingIcon = QIcon(":/settings/images/settings/network.png");

	toggleVisibilityAction = new QAction(tr("Toggle"), this);
	connect(toggleVisibilityAction, SIGNAL(triggered()),
		this, SLOT(ToggleVisibility()));

	stopAction = new QAction(tr("Ignorar OBS Tray"), this);
	connect(stopAction, SIGNAL(triggered()), this, SLOT(hide()));

	setupAction = new QAction(ptbr("Configuração"), this);
	connect(setupAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

	quitAction = new QAction(tr("&Sair"), this);
	connect(quitAction, SIGNAL(triggered()), this, SLOT(closeObsTray()));

	createTrayIcon();

	trayIcon->setIcon(defaultIcon);
	trayIcon->show();

	setWindowTitle(tr("OBSTray"));

	obsRunning = false;
}

void OBSTray::AddClient()
{
	clientWbSocket = wbsServer->nextPendingConnection();
	connect(clientWbSocket, SIGNAL(textMessageReceived(QString)), this, SLOT(ProcessRemoteController(QString)));
}

void OBSTray::ProcessRemoteController(QString str)
{
	//processa comando recebido
	//validação/segurança (?)
	if (QString::compare(str, "prepareOBS") == 0)
		SendPrepareSignal();

	else if (QString::compare(str, "toggleOBS") == 0)
		ToggleVisibility();

	else if (QString::compare(str, "stopStreaming") == 0)
		SendStopStreamingSignal();

	else if (QString::compare(str, "closeOBS") == 0)
		SendCloseSignal();

	else if (QString::compare(str, "relaunchOBS") == 0)
		SendRelunchSignal();
	
	// como dizer pro obs o endereço da transmissão?
	//StartStreaming();
}

void OBSTray::ToggleVisibility(){
	if (obsRunning)
		emit toggleVisibility();
	else{
		QMessageBox::StandardButton result = 
			QMessageBox::question(this, tr("OBS"),
			ptbr("OBS não está em execução\r\nDeseja iniciar o OBS?"),
			QMessageBox::Yes | QMessageBox::No);

		if (result == QMessageBox::Yes)
			SendPrepareSignal();
	}
}


void OBSTray::SendPrepareSignal(){
	// don't open OBS if it is already running
	if (obsRunning) return;

	emit prepareObs();
	obsRunning = true;
}

void OBSTray::SendStartStreamingSignal(){
	if (!obsRunning) return;

	emit startStreaming();
}

void OBSTray::SendStopStreamingSignal(bool close){
	if (!obsRunning) return;
		
	emit stopStreaming();

	if (close)
		SendCloseSignal();
}

void OBSTray::SendCloseSignal(){
	if (!obsRunning) return;

	// how can I close OBS without closing the application?
	emit closeObs();
	obsRunning = false;
}

void OBSTray::SendRelunchSignal(){
	if (!obsRunning)
		SendPrepareSignal();
	else{
		emit relaunchObs();
		emit closeObs();
		obsRunning = false;
	}
		
}

void OBSTray::setVisible(bool visible)
{
	QDialog::setVisible(visible);
}

void OBSTray::closeEvent(QCloseEvent *event)
{
	if (trayIcon->isVisible()) {
		QMessageBox::information(this, tr("OBSTray"),
			tr("OBSTray continuará executando em segundo "
			"plano aguardando o início da transmissão."));
		hide();
		event->ignore();
	}
}

void OBSTray::closeObsTray()
{
	QMessageBox::StandardButton reallyCloseObs;
	reallyCloseObs = QMessageBox::question(this, tr("OBSTray"), "Deseja mesmo sair do OBS?",
		QMessageBox::Yes | QMessageBox::No);
	if (reallyCloseObs == QMessageBox::Yes)
		SendCloseSignal();
}

void OBSTray::setIcon(int index)
{
	QIcon icon = iconComboBox->itemIcon(index);
	trayIcon->setIcon(icon);
	setWindowIcon(icon);

	trayIcon->setToolTip(iconComboBox->itemText(index));
}

void OBSTray::createTrayIcon()
{
	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(toggleVisibilityAction);
	trayIconMenu->addAction(stopAction);
	trayIconMenu->addAction(setupAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayIconMenu);
}